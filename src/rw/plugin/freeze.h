#ifndef RW_PLUGIN_FREEZE_H
#define RW_PLUGIN_FREEZE_H

#include "driver_platform_registry.h"
#include "engine_module_registry.h"
#include "object_registry.h"

namespace rw::plugin {

// freezeAll — глобальная точка "заморозки" всех registries.
//
// Вызывается из Engine::open() (или Engine::start(), в зависимости от того,
// после какой именно фазы все модули/драйверы уже зарегистрировали свои
// плагины — см. Phase 7 в плане).
//
// После freezeAll() любая попытка registerExtension / registerStreamPlugin /
// registerRightsPlugin / registerAlwaysPlugin вернёт
// std::unexpected(RegisterError::frozenRegistry).
//
// Шаблонный параметр CoreOwners... — это compile-time список core RW
// object types, чьи ObjectRegistry-инстансы нужно заморозить. Это нужно,
// потому что ObjectRegistry<T>::instance() — Meyers singleton, и из плагин-
// layer мы не знаем заранее весь список Owner-типов. Конкретный список
// объявляется в одном месте (например в engine.cpp) и передается сюда.
//
// Пример (это код будет в engine.cpp после Phase 7):
//
//   rw::plugin::freezeAll<
//       rw::Frame, rw::Clump, rw::Atomic, rw::Geometry, rw::Material,
//       rw::Texture, rw::Raster, rw::Light, rw::Camera,
//       rw::World, rw::TexDictionary>();
//
template<PluginOwner... CoreOwners>
inline void freezeAll() noexcept
{
    (ObjectRegistry<CoreOwners>::instance().freeze(), ...);
    EngineModuleRegistry::instance().freeze();
    DriverPlatformRegistry::instance().freezeAll();
}

// Удобный alias, если в каком-то TU нужно заморозить только Engine/Driver,
// не трогая object registries (например в тестах).
inline void freezeEngineAndDriver() noexcept
{
    EngineModuleRegistry::instance().freeze();
    DriverPlatformRegistry::instance().freezeAll();
}

}   // namespace rw::plugin

#endif  // RW_PLUGIN_FREEZE_H
