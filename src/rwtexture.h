#pragma once
#ifndef RW_TEXTURE_H
#define RW_TEXTURE_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwobject.h"
#include "rwfwd.h"
#include <array>
#include <type_traits>

namespace rw {

struct Texture
{
	enum FilterMode {
		NEAREST = 1,
		LINEAR,
		MIPNEAREST,	// one mipmap
		MIPLINEAR,
		LINEARMIPNEAREST,	// mipmap interpolated
		LINEARMIPLINEAR
	};
	enum Addressing {
		WRAP = 1,
		MIRROR,
		CLAMP,
		BORDER
	};

	PLUGINBASE
	// Marker for the new typed plugin layer (rw::plugin::PluginOwner concept).
	// Coexists with PLUGINBASE; legacy system is still authoritative until the
	// owner-wide migration completes (see plan Phase 4).
	using plugin_owner_tag = Texture;

	Raster *raster;
	TexDictionary *dict;
	LLLink inDict;
	std::array<char, 32> name;
	std::array<char, 32> mask;
	uint32 filterAddressing; // VVVVUUUU FFFFFFFF
	int32 refCount;

	LLLink inGlobalList;	// actually not in RW

	static int32 numAllocated;

	[[nodiscard]] static Texture *create(Raster *raster);
	void addRef(void) noexcept { this->refCount++; }
	void destroy(void);
	[[nodiscard]] static Texture *fromDict(LLLink *lnk){
		return LLLinkGetData(lnk, Texture, inDict); }
	[[nodiscard]] FilterMode getFilter(void) const noexcept { return (FilterMode)(filterAddressing & 0xFF); }
	void setFilter(FilterMode f) noexcept { filterAddressing = (filterAddressing & ~0xFF) | f; }
	[[nodiscard]] Addressing getAddressU(void) const noexcept { return (Addressing)((filterAddressing >> 8) & 0xF); }
	[[nodiscard]] Addressing getAddressV(void) const noexcept { return (Addressing)((filterAddressing >> 12) & 0xF); }
	void setAddressU(Addressing u) noexcept { filterAddressing = (filterAddressing & ~0xF00) | u<<8; }
	void setAddressV(Addressing v) noexcept { filterAddressing = (filterAddressing & ~0xF000) | v<<12; }
	[[nodiscard]] static Texture *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);
	[[nodiscard]] static Texture *read(const char *name, const char *mask);
	[[nodiscard]] static Texture *streamReadNative(Stream *stream);
	void streamWriteNative(Stream *stream);
	uint32 streamGetSizeNative(void);

	static Texture *(*findCB)(const char *name);
	static Texture *(*readCB)(const char *name, const char *mask);
	static void setLoadTextures(bool32);	// default: true
	static void setCreateDummies(bool32);	// default: false
	static void setMipmapping(bool32);	// default: false
	static void setAutoMipmapping(bool32);	// default: false
	[[nodiscard]] static bool32 getMipmapping(void);
	[[nodiscard]] static bool32 getAutoMipmapping(void);

	void setMaxAnisotropy(int32 maxaniso);	// only if plugin is attached
	[[nodiscard]] int32 getMaxAnisotropy(void);

#ifndef RWPUBLIC
	static void registerModule(void);
#endif
};

void registerAnisotropyPlugin(void);
int32 getMaxSupportedMaxAnisotropy(void);

struct TexDictionary
{
	PLUGINBASE
	using plugin_owner_tag = TexDictionary;
	static inline constexpr int32 ID = 6;
	Object object;
	LinkList textures;
	LLLink inGlobalList;

	static int32 numAllocated;

	[[nodiscard]] static TexDictionary *create(void);
	[[nodiscard]] static TexDictionary *fromLink(LLLink *lnk){
		return LLLinkGetData(lnk, TexDictionary, inGlobalList); }
	void destroy(void);
	[[nodiscard]] int32 count(void) { return this->textures.count(); }
	void add(Texture *t);
	void addFront(Texture *t);
	void remove(Texture *t);
	[[nodiscard]] Texture *find(const char *name);
	[[nodiscard]] static TexDictionary *streamRead(Stream *stream);
	void streamWrite(Stream *stream);
	uint32 streamGetSize(void);

	static void setCurrent(TexDictionary *txd);
	[[nodiscard]] static TexDictionary *getCurrent(void);
};

static_assert(std::is_standard_layout_v<Texture>,       "Texture must be standard-layout for plugin offsets");
static_assert(std::is_standard_layout_v<TexDictionary>, "TexDictionary must be standard-layout for plugin offsets");

}

#endif // RW_TEXTURE_H
