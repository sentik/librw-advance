#ifndef RW_PLUGIN_DESCRIPTOR_H
#define RW_PLUGIN_DESCRIPTOR_H

#include "types.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <source_location>
#include <string_view>

// Forward declaration: rw::Stream живёт в rwbase.h. Descriptor хранит
// std::function<...>(rw::Stream&, ...), но не зависит от полного определения.
namespace rw {
class Stream;
}

namespace rw::plugin {

// Жизненный цикл extension storage.
// Пустой std::function (operator bool == false) трактуется как "skip" в
// runtime traversal. Это убирает defCtor/defDtor/defCopy no-op трамплины.
//
// Сигнатуры не noexcept — std::function в C++23 не поддерживает noexcept
// signature (для этого нужен std::move_only_function<R(Args...) noexcept>,
// который вводится только в C++23 как опциональный path). Реализация
// гарантирует, что все наши callbacks ловят/обрабатывают исключения
// внутри себя; внешний код видит typed std::expected.
struct PluginLifecycle {
    using Ctor = std::function<void(void* owner, std::ptrdiff_t offset)>;
    using Dtor = std::function<void(void* owner, std::ptrdiff_t offset)>;
    using Copy = std::function<void(void* dst, const void* src,
                                    std::ptrdiff_t offset)>;

    Ctor construct;
    Dtor destruct;
    Copy copy;

    [[nodiscard]] bool hasConstruct() const noexcept { return static_cast<bool>(construct); }
    [[nodiscard]] bool hasDestruct()  const noexcept { return static_cast<bool>(destruct); }
    [[nodiscard]] bool hasCopy()      const noexcept { return static_cast<bool>(copy); }
};

// Stream-IO часть descriptor-а. Каждое поле опционально (пустой std::function
// → "не реализовано"). PluginKind помогает валидировать combination на
// registration.
struct PluginStream {
    using Read    = std::function<
        std::expected<void, StreamPluginError>(
            Stream& stream, std::int32_t length,
            void* owner, std::ptrdiff_t offset)>;

    using Write   = std::function<
        std::expected<void, StreamPluginError>(
            Stream& stream, std::int32_t length,
            const void* owner, std::ptrdiff_t offset)>;

    using GetSize = std::function<
        std::int32_t(const void* owner, std::ptrdiff_t offset)>;

    using Rights  = std::function<
        void(void* owner, std::ptrdiff_t offset, std::uint32_t data)>;

    using Always  = std::function<
        void(void* owner, std::ptrdiff_t offset)>;

    Read    read;
    Write   write;
    GetSize getSize;
    Rights  rights;
    Always  always;

    StreamRequirement requirement = StreamRequirement::optional;

    [[nodiscard]] bool hasRead()    const noexcept { return static_cast<bool>(read); }
    [[nodiscard]] bool hasWrite()   const noexcept { return static_cast<bool>(write); }
    [[nodiscard]] bool hasGetSize() const noexcept { return static_cast<bool>(getSize); }
    [[nodiscard]] bool hasRights()  const noexcept { return static_cast<bool>(rights); }
    [[nodiscard]] bool hasAlways()  const noexcept { return static_cast<bool>(always); }
};

// Debug-метаданные. Включаются только в debug build, чтобы не раздувать
// release binary. std::source_location и string_view имеют ненулевой размер,
// поэтому [[no_unique_address]] не поможет.
struct PluginDebugInfo {
    std::string_view     extName;
    std::source_location registeredAt;
};

// Сам descriptor. Один экземпляр на каждый registerExtension /
// registerStreamPlugin / registerRightsPlugin. Хранится в
// std::vector<PluginDescriptor> внутри registry — порядок регистрации
// сохраняется (важно для voidoid construct order и stream chunk order).
struct PluginDescriptor {
    PluginId       id {};
    PluginKind     kind {};
    std::ptrdiff_t offset = 0;
    std::size_t    size   = 0;
    std::size_t    alignment = 1;

    PluginLifecycle lifecycle;
    PluginStream    stream;

#ifndef NDEBUG
    PluginDebugInfo debug;
#endif
};

}   // namespace rw::plugin

#endif  // RW_PLUGIN_DESCRIPTOR_H
