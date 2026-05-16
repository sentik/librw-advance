#ifndef RW_PLUGIN_PLUGIN_LIST_CORE_H
#define RW_PLUGIN_PLUGIN_LIST_CORE_H

#include "descriptor.h"
#include "types.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <utility>
#include <vector>

namespace rw {
class Stream;
}

namespace rw::plugin {

// Type-erased ядро plugin registry. Все три public-обертки
// (ObjectRegistry<Owner>, EngineModuleRegistry, DriverPlatformRegistry)
// делегируют сюда через void* owner pointer.
//
// PluginListCore сохраняет порядок регистрации (construct/destruct/copy/
// stream chunk emission делается в порядке append). После freeze() строится
// sorted lookup table (byId_) для O(log N) поиска по PluginId на горячем
// stream-read пути.
class PluginListCore {
public:
    explicit PluginListCore(std::size_t baseSize) noexcept;

    PluginListCore(const PluginListCore&) = delete;
    PluginListCore& operator=(const PluginListCore&) = delete;

    // Регистрация. Все возвращают typed RegisterError при duplicate id,
    // frozen registry, invalid size/align, missing callbacks etc.

    [[nodiscard]] std::expected<std::ptrdiff_t, RegisterError>
        addStorageExtension(PluginId id,
                            std::size_t size,
                            std::size_t alignment,
                            PluginLifecycle lifecycle,
                            PluginStream stream,
                            PluginDebugInfo debug = {});

    [[nodiscard]] std::expected<void, RegisterError>
        addStreamPlugin(PluginId id,
                        PluginStream stream,
                        PluginDebugInfo debug = {});

    [[nodiscard]] std::expected<void, RegisterError>
        addRightsPlugin(PluginId id,
                        PluginStream::Rights rights,
                        PluginDebugInfo debug = {});

    [[nodiscard]] std::expected<void, RegisterError>
        addAlwaysPlugin(PluginId id,
                        PluginStream::Always always,
                        PluginDebugInfo debug = {});

    // Layout.
    [[nodiscard]] std::size_t baseSize()   const noexcept { return baseSize_; }
    [[nodiscard]] std::size_t objectSize() const noexcept { return objectSize_; }
    [[nodiscard]] bool        isFrozen()   const noexcept
    {
        return frozen_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::uint32_t generation() const noexcept { return generation_; }

    // freeze: запрещает дальнейшую регистрацию, строит byId_ lookup.
    // Идемпотентно — повторный вызов no-op.
    void freeze() noexcept;

    // Lifecycle traversal. Owner — void* по type erasure; вызывающий должен
    // гарантировать, что это указатель на тот же Owner-тип, для которого
    // создан этот registry.
    void construct(void* owner) const noexcept;
    void destruct (void* owner) const noexcept;
    void copyAll  (void* dst, const void* src) const noexcept;

    // Stream IO.
    [[nodiscard]] std::expected<void, StreamPluginError>
        streamRead (Stream& stream, void* owner) const noexcept;

    [[nodiscard]] std::expected<void, StreamPluginError>
        streamWrite(Stream& stream, const void* owner) const noexcept;

    [[nodiscard]] std::int32_t streamGetSize(const void* owner) const noexcept;

    void streamSkip(Stream& stream) const noexcept;

    void assertRights(void* owner, PluginId id, std::uint32_t data) const noexcept;

    // Introspection.
    [[nodiscard]] std::span<const PluginDescriptor> descriptors() const noexcept;
    [[nodiscard]] const PluginDescriptor* find(PluginId id) const noexcept;

private:
    [[nodiscard]] std::expected<void, RegisterError>
        checkRegisterPrecondition(PluginId id) const noexcept;

    std::vector<PluginDescriptor>                  descriptors_;
    // sorted by PluginId после freeze; во время registration не используется.
    std::vector<std::pair<PluginId, std::size_t>>  byId_;
    std::size_t   baseSize_;
    std::size_t   objectSize_;
    std::uint32_t generation_ = 0;
    std::atomic<bool> frozen_{false};
};

}   // namespace rw::plugin

#endif  // RW_PLUGIN_PLUGIN_LIST_CORE_H
