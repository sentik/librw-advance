#ifndef RW_PLUGIN_ENGINE_MODULE_REGISTRY_H
#define RW_PLUGIN_ENGINE_MODULE_REGISTRY_H

#include "concepts.h"
#include "descriptor.h"
#include "plugin_list_core.h"
#include "types.h"

#include <cstddef>
#include <expected>
#include <source_location>
#include <string_view>
#include <utility>

namespace rw {
class Stream;
}

namespace rw::plugin {

// Singleton-registry для Engine-wide плагинов и модулей.
//
// Заменяет старый Engine::s_plglist. Owner — это сам Engine; имеет смысл
// единственный инстанс на процесс.
//
// Используется для:
//   - Engine module init/shutdown (Frame, Image, Raster, Texture, HAnim,
//     UVAnim, PDS);
//   - Engine globals storage (ImageGlobals, RasterGlobals, TextureGlobals).
//
// Lifetime: создается при первом обращении, freeze-ится в freezeAll()
// после Engine::open() / Engine::start().
class EngineModuleRegistry {
public:
    [[nodiscard]] static EngineModuleRegistry& instance() noexcept;

    EngineModuleRegistry(const EngineModuleRegistry&) = delete;
    EngineModuleRegistry& operator=(const EngineModuleRegistry&) = delete;

    // Storage extension: для модулей, у которых есть Globals struct.
    // size и alignment берутся из типа через шаблон.
    template<PluginExtension Ext>
    [[nodiscard]] std::expected<std::ptrdiff_t, RegisterError>
    registerModuleStorage(PluginId id,
                          PluginLifecycle lifecycle,
                          std::string_view name = {},
                          std::source_location loc =
                              std::source_location::current())
    {
        return core_.addStorageExtension(
            id, sizeof(Ext), alignof(Ext),
            std::move(lifecycle), /*stream*/ {},
            PluginDebugInfo{name, loc});
    }

    // Для модулей без storage (только init/shutdown через construct/destruct).
    [[nodiscard]] std::expected<void, RegisterError>
    registerModuleLifecycle(PluginId id,
                            PluginLifecycle lifecycle,
                            std::string_view name = {},
                            std::source_location loc =
                                std::source_location::current());

    // Layout / freeze.
    [[nodiscard]] std::size_t engineSize() const noexcept { return core_.objectSize(); }
    [[nodiscard]] bool        isFrozen()   const noexcept { return core_.isFrozen(); }
    void freeze() noexcept { core_.freeze(); }

    // Lifecycle (вызывается из Engine::open/close).
    void construct(void* engine) const noexcept { core_.construct(engine); }
    void destruct (void* engine) const noexcept { core_.destruct (engine); }

    // Introspection.
    [[nodiscard]] std::span<const PluginDescriptor> descriptors() const noexcept
    {
        return core_.descriptors();
    }

    [[nodiscard]] PluginListCore& core() noexcept { return core_; }

private:
    EngineModuleRegistry() noexcept;

    PluginListCore core_;
};

}   // namespace rw::plugin

#endif  // RW_PLUGIN_ENGINE_MODULE_REGISTRY_H
