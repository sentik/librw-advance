// Compile-time smoke checks: prove the new typed plugin layer composes
// correctly with real librw types from the legacy header chain.
//
// No runtime code, no exported symbols. Pure static_assert plumbing.
// Lives in a separate TU so it doesn't clutter plugin_list_core.cpp.

#include "concepts.h"
#include "descriptor.h"
#include "object_registry.h"
#include "offset.h"
#include "types.h"

#include "../../rwbase.h"
#include "../../rwerror.h"
#include "../../rwplg.h"
#include "../../rwpipeline.h"
#include "../../rwobjects.h"

#include <cstdint>
#include <type_traits>

namespace rw::plugin::smoke {

// PluginId arithmetic must equal the legacy MAKEPLUGINID layout bit-for-bit.
// VEND_CORE=0, VEND_CRITERIONTK=1.
static_assert(toRaw(makePluginId(1u, 0x16u)) ==
              static_cast<std::uint32_t>(ID_SKIN),
              "PluginId(MAKEPLUGINID-equivalent) must match enum PluginID layout");
static_assert(toRaw(makePluginId(1u, 0x1Fu)) ==
              static_cast<std::uint32_t>(ID_USERDATA),
              "PluginId for user data must match legacy ID_USERDATA");
static_assert(toRaw(makePluginId(1u, 0x27u)) ==
              static_cast<std::uint32_t>(ID_ANISOT),
              "PluginId for anisotropy must match legacy ID_ANISOT");

// Texture has been tagged with plugin_owner_tag — confirm the concept is
// satisfied. Other core types still need their tags added in Phase 4.
static_assert(PluginOwner<rw::Texture>,
              "rw::Texture must satisfy the PluginOwner concept");

// Standard scalar extensions used by current plugins.
static_assert(TrivialExtension<std::int32_t>);
static_assert(TrivialExtension<std::uint32_t>);
static_assert(TrivialExtension<void*>);

// PluginOffset is default-constructible to an invalid state and is
// owner/ext-typed. Sanity check size & operator bool.
static_assert(sizeof(PluginOffset<rw::Texture, std::int32_t>) ==
              sizeof(std::ptrdiff_t),
              "PluginOffset is the size of its underlying ptrdiff_t");
static_assert(!PluginOffset<rw::Texture, std::int32_t>{},
              "default-constructed PluginOffset is invalid (operator bool == false)");

// ObjectRegistry<Texture> must instantiate. The body lives in the template
// header; we verify by taking the address of the singleton accessor.
constexpr auto* kTextureRegistryAccessor =
    &ObjectRegistry<rw::Texture>::instance;
static_assert(kTextureRegistryAccessor != nullptr);

// Descriptor invariants.
static_assert(std::is_default_constructible_v<PluginLifecycle>);
static_assert(std::is_default_constructible_v<PluginStream>);
static_assert(std::is_move_constructible_v<PluginDescriptor>);
static_assert(!std::is_copy_constructible_v<PluginListCore>,
              "PluginListCore must not be copyable");

}   // namespace rw::plugin::smoke
