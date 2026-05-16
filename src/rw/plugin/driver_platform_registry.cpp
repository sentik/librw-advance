#include "driver_platform_registry.h"

#include "../../rwbase.h"
#include "../../rwerror.h"
#include "../../rwplg.h"
#include "../../rwpipeline.h"
#include "../../rwobjects.h"
#include "../../rwengine.h"  // rw::Driver

#include <cstddef>
#include <utility>

namespace rw::plugin {

DriverPlatformRegistry::DriverPlatformRegistry() noexcept
    : baseDriverSize_(sizeof(rw::Driver))
{
}

DriverPlatformRegistry& DriverPlatformRegistry::instance() noexcept
{
    static DriverPlatformRegistry r{};
    return r;
}

PluginListCore* DriverPlatformRegistry::coreFor(Platform p) noexcept
{
    const auto idx = static_cast<std::size_t>(toRaw(p));
    if(idx >= slots_.size())
        return nullptr;
    if(!slots_[idx])
        slots_[idx] = std::make_unique<PluginListCore>(baseDriverSize_);
    return slots_[idx].get();
}

const PluginListCore* DriverPlatformRegistry::coreFor(Platform p) const noexcept
{
    const auto idx = static_cast<std::size_t>(toRaw(p));
    if(idx >= slots_.size())
        return nullptr;
    return slots_[idx].get();
}

std::size_t DriverPlatformRegistry::driverSize(Platform p) const noexcept
{
    const auto* core = coreFor(p);
    return core != nullptr ? core->objectSize() : baseDriverSize_;
}

bool DriverPlatformRegistry::isFrozen(Platform p) const noexcept
{
    const auto* core = coreFor(p);
    return core != nullptr ? core->isFrozen() : false;
}

void DriverPlatformRegistry::freezeAll() noexcept
{
    for(auto& slot : slots_){
        if(slot)
            slot->freeze();
    }
}

std::expected<void, RegisterError>
DriverPlatformRegistry::registerPlatformLifecycle(Platform p, PluginId id,
                                                  PluginLifecycle lifecycle,
                                                  std::string_view name,
                                                  std::source_location loc)
{
    auto* core = coreFor(p);
    if(core == nullptr)
        return std::unexpected(RegisterError::invalidPlatform);
    // Используем 0-sized tag — см. комментарий в engine_module_registry.cpp.
    // В Phase 7 это упрощение заменяется явным lifecycle-only descriptor kind.
    struct EmptyTag {};
    auto result = core->addStorageExtension(
        id, sizeof(EmptyTag), alignof(EmptyTag),
        std::move(lifecycle), /*stream*/ {},
        PluginDebugInfo{name, loc});
    if(!result)
        return std::unexpected(result.error());
    return {};
}

void DriverPlatformRegistry::construct(Platform p, void* driver) const noexcept
{
    if(const auto* core = coreFor(p))
        core->construct(driver);
}

void DriverPlatformRegistry::destruct(Platform p, void* driver) const noexcept
{
    if(const auto* core = coreFor(p))
        core->destruct(driver);
}

std::expected<void, StreamPluginError>
DriverPlatformRegistry::streamRead(Platform p, Stream& s, void* driver) const noexcept
{
    const auto* core = coreFor(p);
    if(core == nullptr)
        return std::unexpected(StreamPluginError::missingExtensionChunk);
    return core->streamRead(s, driver);
}

std::expected<void, StreamPluginError>
DriverPlatformRegistry::streamWrite(Platform p, Stream& s, const void* driver) const noexcept
{
    const auto* core = coreFor(p);
    if(core == nullptr)
        return std::unexpected(StreamPluginError::ioFailure);
    return core->streamWrite(s, driver);
}

}   // namespace rw::plugin
