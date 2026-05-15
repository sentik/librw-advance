#pragma once
#ifndef RW_TEXTURE_H
#define RW_TEXTURE_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwobject.h"
#include "rwfwd.h"

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
	Raster *raster;
	TexDictionary *dict;
	LLLink inDict;
	char name[32];
	char mask[32];
	uint32 filterAddressing; // VVVVUUUU FFFFFFFF
	int32 refCount;

	LLLink inGlobalList;	// actually not in RW

	static int32 numAllocated;

	static Texture *create(Raster *raster);
	void addRef(void) { this->refCount++; }
	void destroy(void);
	static Texture *fromDict(LLLink *lnk){
		return LLLinkGetData(lnk, Texture, inDict); }
	FilterMode getFilter(void) { return (FilterMode)(filterAddressing & 0xFF); }
	void setFilter(FilterMode f) { filterAddressing = (filterAddressing & ~0xFF) | f; }
	Addressing getAddressU(void) { return (Addressing)((filterAddressing >> 8) & 0xF); }
	Addressing getAddressV(void) { return (Addressing)((filterAddressing >> 12) & 0xF); }
	void setAddressU(Addressing u) { filterAddressing = (filterAddressing & ~0xF00) | u<<8; }
	void setAddressV(Addressing v) { filterAddressing = (filterAddressing & ~0xF000) | v<<12; }
	static Texture *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);
	static Texture *read(const char *name, const char *mask);
	static Texture *streamReadNative(Stream *stream);
	void streamWriteNative(Stream *stream);
	uint32 streamGetSizeNative(void);

	static Texture *(*findCB)(const char *name);
	static Texture *(*readCB)(const char *name, const char *mask);
	static void setLoadTextures(bool32);	// default: true
	static void setCreateDummies(bool32);	// default: false
	static void setMipmapping(bool32);	// default: false
	static void setAutoMipmapping(bool32);	// default: false
	static bool32 getMipmapping(void);
	static bool32 getAutoMipmapping(void);

	void setMaxAnisotropy(int32 maxaniso);	// only if plugin is attached
	int32 getMaxAnisotropy(void);

#ifndef RWPUBLIC
	static void registerModule(void);
#endif
};

extern int32 anisotOffset;
#define GETANISOTROPYEXT(texture) PLUGINOFFSET(int32, texture, rw::anisotOffset)
void registerAnisotropyPlugin(void);
int32 getMaxSupportedMaxAnisotropy(void);

struct TexDictionary
{
	PLUGINBASE
	enum { ID = 6 };
	Object object;
	LinkList textures;
	LLLink inGlobalList;

	static int32 numAllocated;

	static TexDictionary *create(void);
	static TexDictionary *fromLink(LLLink *lnk){
		return LLLinkGetData(lnk, TexDictionary, inGlobalList); }
	void destroy(void);
	int32 count(void) { return this->textures.count(); }
	void add(Texture *t);
	void addFront(Texture *t);
	void remove(Texture *t);
	Texture *find(const char *name);
	static TexDictionary *streamRead(Stream *stream);
	void streamWrite(Stream *stream);
	uint32 streamGetSize(void);

	static void setCurrent(TexDictionary *txd);
	static TexDictionary *getCurrent(void);
};

}

#endif // RW_TEXTURE_H
