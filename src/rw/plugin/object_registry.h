#ifndef RW_PLUGIN_OBJECT_REGISTRY_H
#define RW_PLUGIN_OBJECT_REGISTRY_H

#include "concepts.h"
#include "descriptor.h"
#include "offset.h"
#include "plugin_list_core.h"
#include "types.h"

#include <cstddef>
#include <expected>
#include <memory>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>

namespace rw {
class Stream;
}

namespace rw::plugin {

// Per-owner-type plugin registry.
//
// Каждый core RW object (Frame, Clump, Atomic, Geometry, Material, Texture,
// Raster, Light, Camera, World, TexDictionary) получает свой собственный
// инстанс ObjectRegistry<Owner>. Это template-обертка над type-erased
// PluginListCore: вся логика хранения descriptor-ов, alignment, freeze,
// stream IO живёт в .cpp; здесь только typed shims.
//
// Использование:
//
//   auto& reg = rw::plugin::ObjectRegistry<rw::Geometry>::instance();
//   auto off = reg.registerExtension<Skin*>(
//                  rw::plugin::fromRaw(rw::ID_SKIN),
//                  rw::plugin::PluginLifecycle{
//                      .construct = [](void* o, std::ptrdiff_t off){ ... },
//                      .destruct  = [](void* o, std::ptrdiff_t off){ ... },
//                      .copy      = [](void* d, const void* s, std::ptrdiff_t off){ ... },
//                  },
//                  /*stream*/ {},
//                  "Skin");
//   if(!off) /* handle RegisterError */;
//   skinGeometryOffset = *off;
//
// Доступ позже:
//   Skin*& skin = rw::plugin::extension(geometry, skinGeometryOffset);
//
template<PluginOwner Owner>
class ObjectRegistry {
public:
    [[nodiscard]] static ObjectRegistry& instance() noexcept
    {
        // Meyers singleton — гарантированно инициализируется до первого
        // вызова. Регистрации происходят из Engine::open() / module init
        // callbacks, не из global ctors, поэтому SIOF не возникает.
        static ObjectRegistry r{};
        return r;
    }

    ObjectRegistry(const ObjectRegistry&) = delete;
    ObjectRegistry& operator=(const ObjectRegistry&) = delete;

    // Регистрация storage extension с typed Ext. size и alignment
    // выводятся из типа.
    template<PluginExtension Ext>
    [[nodiscard]] std::expected<PluginOffset<Owner, Ext>, RegisterError>
    registerExtension(PluginId id,
                      PluginLifecycle lifecycle,
                      PluginStream stream = {},
                      std::string_view name = {},
                      std::source_location loc =
                          std::source_location::current())
    {
        auto result = core_.addStorageExtension(
            id, sizeof(Ext), alignof(Ext),
            std::move(lifecycle), std::move(stream),
            PluginDebugInfo{name, loc});
        if(!result)
            return std::unexpected(result.error());
        return PluginOffset<Owner, Ext>{*result};
    }

    [[nodiscard]] std::expected<void, RegisterError>
    registerStreamPlugin(PluginId id,
                         PluginStream stream,
                         std::string_view name = {},
                         std::source_location loc =
                             std::source_location::current())
    {
        return core_.addStreamPlugin(id, std::move(stream),
                                     PluginDebugInfo{name, loc});
    }

    [[nodiscard]] std::expected<void, RegisterError>
    registerRightsPlugin(PluginId id,
                         PluginStream::Rights rights,
                         std::string_view name = {},
                         std::source_location loc =
                             std::source_location::current())
    {
        return core_.addRightsPlugin(id, std::move(rights),
                                     PluginDebugInfo{name, loc});
    }

    [[nodiscard]] std::expected<void, RegisterError>
    registerAlwaysPlugin(PluginId id,
                         PluginStream::Always always,
                         std::string_view name = {},
                         std::source_location loc =
                             std::source_location::current())
    {
        return core_.addAlwaysPlugin(id, std::move(always),
                                     PluginDebugInfo{name, loc});
    }

    // Layout.
    [[nodiscard]] std::size_t objectSize() const noexcept { return core_.objectSize(); }
    [[nodiscard]] std::size_t baseSize()   const noexcept { return core_.baseSize(); }
    [[nodiscard]] bool        isFrozen()   const noexcept { return core_.isFrozen(); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return core_.generation(); }

    void freeze() noexcept { core_.freeze(); }

    // Lifecycle.
    void construct(Owner& owner) const noexcept { core_.construct(&owner); }
    void destruct (Owner& owner) const noexcept { core_.destruct (&owner); }
    void copyAll  (Owner& dst, const Owner& src) const noexcept
    {
        core_.copyAll(&dst, &src);
    }

    // Stream IO.
    [[nodiscard]] std::expected<void, StreamPluginError>
    streamRead(Stream& s, Owner& owner) const noexcept
    {
        return core_.streamRead(s, &owner);
    }

    [[nodiscard]] std::expected<void, StreamPluginError>
    streamWrite(Stream& s, const Owner& owner) const noexcept
    {
        return core_.streamWrite(s, &owner);
    }

    [[nodiscard]] std::int32_t streamGetSize(const Owner& owner) const noexcept
    {
        return core_.streamGetSize(&owner);
    }

    void streamSkip(Stream& s) const noexcept { core_.streamSkip(s); }

    void assertRights(Owner& owner, PluginId id, std::uint32_t data) const noexcept
    {
        core_.assertRights(&owner, id, data);
    }

    // Introspection.
    [[nodiscard]] std::span<const PluginDescriptor> descriptors() const noexcept
    {
        return core_.descriptors();
    }

    [[nodiscard]] const PluginDescriptor* find(PluginId id) const noexcept
    {
        return core_.find(id);
    }

    // Низкоуровневый доступ к ядру (для freeze.h и тестов).
    [[nodiscard]] PluginListCore& core() noexcept { return core_; }

private:
    ObjectRegistry() noexcept : core_(sizeof(Owner)) {}

    PluginListCore core_;
};

// Helper: default lifecycle для нетривиального Ext.
//
// Использует std::construct_at / std::destroy_at для placement init/destroy
// и operator= для copy. Owner шаблон-параметр здесь только для compile-time
// проверки concept-ов; в реализации он не используется, потому что лямбды
// делают чистую byte-арифметику от void* owner + offset.
//
// Применять, когда Ext нетривиален. Для trivial Ext entries можно оставить
// пустыми — PluginListCore пропустит их.
template<PluginOwner Owner, PluginExtension Ext>
[[nodiscard]] inline PluginLifecycle defaultLifecycleFor()
{
    return PluginLifecycle{
        .construct = [](void* o, std::ptrdiff_t off) {
            auto* bytes = static_cast<std::byte*>(o);
            std::construct_at(reinterpret_cast<Ext*>(bytes + off));
        },
        .destruct = [](void* o, std::ptrdiff_t off) {
            auto* bytes = static_cast<std::byte*>(o);
            std::destroy_at(
                std::launder(reinterpret_cast<Ext*>(bytes + off)));
        },
        .copy = [](void* d, const void* s, std::ptrdiff_t off) {
            auto*       dbytes = static_cast<std::byte*>(d);
            const auto* sbytes = static_cast<const std::byte*>(s);
            auto* dst = std::launder(reinterpret_cast<Ext*>(dbytes + off));
            const auto* src =
                std::launder(reinterpret_cast<const Ext*>(sbytes + off));
            *dst = *src;
        },
    };
}

}   // namespace rw::plugin

#endif  // RW_PLUGIN_OBJECT_REGISTRY_H
