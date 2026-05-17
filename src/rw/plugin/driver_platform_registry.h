#ifndef RW_PLUGIN_DRIVER_PLATFORM_REGISTRY_H
#define RW_PLUGIN_DRIVER_PLATFORM_REGISTRY_H

#include "concepts.h"
#include "descriptor.h"
#include "plugin_list_core.h"
#include "types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <source_location>
#include <string_view>
#include <utility>

namespace rw {
class Stream;
}

namespace rw::plugin {

// Количество слотов для платформ. Соответствует NUM_PLATFORMS из rwbase.h
// (PLATFORM_GL3 + 1 = 13). Запас держим до 16, чтобы хватило на будущее
// без рекомпиляции зависимого кода.
inline constexpr std::size_t kMaxPlatforms = 16;

// Singleton-registry для платформенных плагинов (Driver::s_plglist[NUM_PLATFORMS]).
//
// Используется для:
//   - регистрации платформенного маркера (платформа есть в сборке);
//   - регистрации платформенного skin/matfx implementation;
//   - регистрации native raster extension.
//
// Внутри держит std::array<PluginListCore, kMaxPlatforms>. Каждый платформенный
// слот ленивo инициализируется при первом обращении (PluginListCore
// non-default-constructible — поэтому через std::unique_ptr).
class DriverPlatformRegistry {
public:
    [[nodiscard]] static DriverPlatformRegistry& instance() noexcept;

    DriverPlatformRegistry(const DriverPlatformRegistry&) = delete;
    DriverPlatformRegistry& operator=(const DriverPlatformRegistry&) = delete;

    // Базовый размер Driver-структуры. Все registerPlatformExtension
    // увеличивают этот размер.
    // p accepts legacy rw::Platform integer values implicitly.
    [[nodiscard]] std::size_t driverSize(int p) const noexcept;

    [[nodiscard]] bool isFrozen(int p) const noexcept;

    void freezeAll() noexcept;

    // Регистрация storage extension для конкретной платформы.
    // Платформенно-специфичная storage (D3dRaster, GlRaster, XboxRaster, ...).
    template<PluginExtension Ext>
    [[nodiscard]] std::expected<std::ptrdiff_t, RegisterError>
    registerPlatformStorage(int p, PluginId id,
                            PluginLifecycle lifecycle,
                            PluginStream stream = {},
                            std::string_view name = {},
                            std::source_location loc =
                                std::source_location::current())
    {
        auto* core = coreFor(p);
        if(core == nullptr)
            return std::unexpected(RegisterError::invalidPlatform);
        return core->addStorageExtension(
            id, sizeof(Ext), alignof(Ext),
            std::move(lifecycle), std::move(stream),
            PluginDebugInfo{name, loc});
    }

    // Регистрация без storage — для платформенных markers (skin/matfx init).
    [[nodiscard]] std::expected<void, RegisterError>
    registerPlatformLifecycle(int p, PluginId id,
                              PluginLifecycle lifecycle,
                              std::string_view name = {},
                              std::source_location loc =
                                  std::source_location::current());

    // Lifecycle (вызывается при Driver init/teardown per platform).
    void construct(int p, void* driver) const noexcept;
    void destruct (int p, void* driver) const noexcept;

    // Stream IO (например native raster data).
    [[nodiscard]] std::expected<void, StreamPluginError>
    streamRead(int p, Stream& s, void* driver) const noexcept;

    [[nodiscard]] std::expected<void, StreamPluginError>
    streamWrite(int p, Stream& s, const void* driver) const noexcept;

    // Introspection.
    [[nodiscard]] PluginListCore* coreFor(int p) noexcept;
    [[nodiscard]] const PluginListCore* coreFor(int p) const noexcept;

private:
    DriverPlatformRegistry() noexcept;

    // Один core на платформу; nullptr означает "платформа не активна
    // (не было ни одной регистрации)".
    std::array<std::unique_ptr<PluginListCore>, kMaxPlatforms> slots_;

    std::size_t baseDriverSize_;   // sizeof(rw::Driver), захватывается в ctor.
};

}   // namespace rw::plugin

#endif  // RW_PLUGIN_DRIVER_PLATFORM_REGISTRY_H
