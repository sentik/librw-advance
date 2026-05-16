#include "engine_module_registry.h"

#include "../../rwbase.h"
#include "../../rwerror.h"
#include "../../rwplg.h"
#include "../../rwpipeline.h"
#include "../../rwobjects.h"
#include "../../rwengine.h"   // rw::Engine, для sizeof(Engine) baseline

#include <utility>

namespace rw::plugin {

EngineModuleRegistry::EngineModuleRegistry() noexcept
    : core_(sizeof(rw::Engine))
{
}

EngineModuleRegistry& EngineModuleRegistry::instance() noexcept
{
    static EngineModuleRegistry r{};
    return r;
}

std::expected<void, RegisterError>
EngineModuleRegistry::registerModuleLifecycle(PluginId id,
                                              PluginLifecycle lifecycle,
                                              std::string_view name,
                                              std::source_location loc)
{
    // Размер 0 — это не storage extension; формально это streamOnly с
    // construct/destruct callbacks. Используем нижележащий addStreamPlugin
    // и потом приписываем lifecycle вручную.
    //
    // Для модулей без storage важно, чтобы construct/destruct вызывались
    // на Engine* при open/close. Это делается через PluginListCore::construct
    // / destruct в обычном порядке.
    //
    // Чтобы lifecycle вызывался, нам нужно записать его в descriptor.
    // addStreamPlugin создает descriptor с пустым lifecycle, что не подходит.
    // Поэтому используем addStorageExtension с size==0 — но это запрещено
    // через storageRequiredButZero. Альтернатива: расширить PluginListCore
    // отдельным `addLifecycleOnlyPlugin`.
    //
    // На время Phase 1 модули без storage обрабатываются через
    // construct/destruct полей нулевого размера в registerModuleStorage<EmptyTag>.
    // Это упрощение убирается в Phase 7 при миграции реальных Engine modules.
    struct EmptyTag {};
    auto result = core_.addStorageExtension(
        id, sizeof(EmptyTag), alignof(EmptyTag),
        std::move(lifecycle), /*stream*/ {},
        PluginDebugInfo{name, loc});
    if(!result)
        return std::unexpected(result.error());
    return {};
}

}   // namespace rw::plugin
