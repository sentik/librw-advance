#ifndef RW_PLUGIN_OFFSET_H
#define RW_PLUGIN_OFFSET_H

#include "concepts.h"

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>

namespace rw::plugin {

template<PluginOwner Owner>
class ObjectRegistry;     // fwd, friend для PluginOffset

// alignUp(value, alignment).
// Alignment должен быть степенью двойки. В debug это assert,
// в release — [[assume]] для оптимизатора.
[[nodiscard]] constexpr std::size_t alignUp(std::size_t value,
                                            std::size_t alignment) noexcept
{
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0);
#if defined(__cpp_assume) && __cpp_assume >= 202207L
    [[assume(alignment != 0)]];
    [[assume((alignment & (alignment - 1)) == 0)]];
#endif
    return (value + alignment - 1) & ~(alignment - 1);
}

// Typed offset для extension storage. Owner и Ext mismatch — compile error.
// Конструктор приватный; единственный способ получить корректный offset —
// через ObjectRegistry<Owner>::registerExtension<Ext>.
//
// value_ == -1 означает "не инициализирован". Все offset access
// функции assert-ятся, что offset валиден.
template<PluginOwner Owner, PluginExtension Ext>
class PluginOffset {
public:
    constexpr PluginOffset() noexcept = default;

    [[nodiscard]] explicit constexpr operator bool() const noexcept
    {
        return value_ >= 0;
    }

    [[nodiscard]] constexpr std::ptrdiff_t value() const noexcept
    {
        assert(value_ >= 0);
#if defined(__cpp_assume) && __cpp_assume >= 202207L
        [[assume(value_ >= 0)]];
#endif
        return value_;
    }

private:
    friend class ObjectRegistry<Owner>;

    explicit constexpr PluginOffset(std::ptrdiff_t v) noexcept : value_(v) {}

    std::ptrdiff_t value_ = -1;
};

// Typed access к extension storage. Owner mismatch и Ext mismatch — compile
// error. std::launder корректно "открывает" lifetime для объектов, созданных
// через placement new в tail storage.
template<PluginOwner Owner, PluginExtension Ext>
[[nodiscard]] inline Ext& extension(Owner& owner,
                                    PluginOffset<Owner, Ext> off) noexcept
{
    const auto byteOffset = off.value();
    auto* bytes = std::bit_cast<std::byte*>(&owner);
    auto* slot  = reinterpret_cast<Ext*>(bytes + byteOffset);
    assert(reinterpret_cast<std::uintptr_t>(slot) % alignof(Ext) == 0
           && "extension storage misaligned");
    return *std::launder(slot);
}

template<PluginOwner Owner, PluginExtension Ext>
[[nodiscard]] inline const Ext& extension(const Owner& owner,
                                          PluginOffset<Owner, Ext> off) noexcept
{
    const auto byteOffset = off.value();
    const auto* bytes = std::bit_cast<const std::byte*>(&owner);
    const auto* slot  = reinterpret_cast<const Ext*>(bytes + byteOffset);
    assert(reinterpret_cast<std::uintptr_t>(slot) % alignof(Ext) == 0
           && "extension storage misaligned");
    return *std::launder(slot);
}

// Указатель-вариант для совместимости с call sites которые держат Owner*.
template<PluginOwner Owner, PluginExtension Ext>
[[nodiscard]] inline Ext* extensionPtr(Owner* owner,
                                       PluginOffset<Owner, Ext> off) noexcept
{
    assert(owner != nullptr);
    return &extension<Owner, Ext>(*owner, off);
}

template<PluginOwner Owner, PluginExtension Ext>
[[nodiscard]] inline const Ext* extensionPtr(const Owner* owner,
                                             PluginOffset<Owner, Ext> off) noexcept
{
    assert(owner != nullptr);
    return &extension<Owner, Ext>(*owner, off);
}

}   // namespace rw::plugin

#endif  // RW_PLUGIN_OFFSET_H
