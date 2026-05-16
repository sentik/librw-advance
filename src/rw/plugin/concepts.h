#ifndef RW_PLUGIN_CONCEPTS_H
#define RW_PLUGIN_CONCEPTS_H

#include <concepts>
#include <type_traits>

namespace rw::plugin {

namespace detail {

// Forward declaration of the traits hook. Каждый core RW object (Frame, Clump,
// Atomic, Geometry, Material, Texture, Raster, Light, Camera, ...) должен
// предоставить специализацию rw::object::ObjectTraits<T> с typedef tag_t.
// Концепт PluginOwner проверяет наличие этой специализации, что превращает
// "забыл зарегистрировать owner" из runtime-сбоя в compile error.
template<class T>
concept HasObjectTraits = requires {
    typename T::plugin_owner_tag;
};

}   // namespace detail

// Owner — это core RW object с tail storage для plugin extensions.
// Требования:
//  - standard layout (необходимо для безопасной pointer-арифметики
//    extension storage);
//  - предоставляет inner typedef `plugin_owner_tag`, который сигнализирует
//    "я зарегистрированный owner". Tag должен совпадать с типом самого owner
//    (это пара-проверка от случайной dependency injection).
template<class T>
concept PluginOwner = std::is_standard_layout_v<T> &&
                      detail::HasObjectTraits<T>;

// Extension — это пользовательский тип, который живёт в tail storage owner-а.
// Ограничения:
//  - destructor noexcept (текущий plugin lifecycle единый, exceptions через
//    него запрещены, см. plan §13);
//  - alignment до 64 байт (cache line). Большее alignment в tail storage
//    осмысленно не имеет — основной owner аллоцируется через rwMalloc и
//    выровнен максимум до alignof(std::max_align_t).
template<class E>
concept PluginExtension = std::is_nothrow_destructible_v<E> &&
                          (alignof(E) <= 64);

// Trivial extension может пропускать construct/destruct/copy callbacks
// в дескрипторе — все три операции эквивалентны no-op или memcpy, что и
// делает текущий defCtor/defDtor/defCopy трамплин.
template<class E>
concept TrivialExtension = PluginExtension<E> &&
                           std::is_trivially_default_constructible_v<E> &&
                           std::is_trivially_copyable_v<E> &&
                           std::is_trivially_destructible_v<E>;

}   // namespace rw::plugin

#endif  // RW_PLUGIN_CONCEPTS_H
