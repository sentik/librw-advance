#pragma once
#ifndef RW_RASTER_H
#define RW_RASTER_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwfwd.h"

namespace rw {

// used to emulate d3d and xbox textures
struct RasterLevels
{
	int32 numlevels;
	uint32 format;
	struct Level {
		int32 width, height, size;
		uint8 *data;
	} levels[1];	// 0 is illegal :/
};

struct Raster
{
	enum { FLIPWAITVSYNCH = 1 };

	PLUGINBASE
	int32 platform;

	// TODO: use bytes
	int32 type;
	int32 flags;
	int32 privateFlags;
	int32 format;
	int32 width, height, depth;
	int32 stride;
	uint8 *pixels;
	uint8 *palette;
	// remember for locked rasters
	uint8 *originalPixels;
	int32 originalWidth;
	int32 originalHeight;
	int32 originalStride;
	// subraster
	Raster *parent;
	int32 offsetX, offsetY;

	static int32 numAllocated;

	[[nodiscard]] static Raster *create(int32 width, int32 height, int32 depth,
	                                    int32 format, int32 platform = 0);
	void subRaster(Raster *parent, Rect *r);
	void destroy(void);
	[[nodiscard]] static bool32 imageFindRasterFormat(Image *image, int32 type,
		int32 *pWidth, int32 *pHeight, int32 *pDepth, int32 *pFormat, int32 platform = 0);
	[[nodiscard]] Raster *setFromImage(Image *image, int32 platform = 0);
	[[nodiscard]] static Raster *createFromImage(Image *image, int32 platform = 0);
	[[nodiscard]] Image *toImage(void);
	[[nodiscard]] uint8 *lock(int32 level, int32 lockMode);
	void unlock(int32 level);
	[[nodiscard]] uint8 *lockPalette(int32 lockMode);
	void unlockPalette(void);
	[[nodiscard]] int32 getNumLevels(void);
	[[nodiscard]] static int32 calculateNumLevels(int32 width, int32 height);
	[[nodiscard]] static bool formatHasAlpha(int32 format);

	void show(uint32 flags);

	[[nodiscard]] static Raster *pushContext(Raster *raster);
	[[nodiscard]] static Raster *popContext(void);
	[[nodiscard]] static Raster *getCurrentContext(void);
	[[nodiscard]] bool32 renderFast(int32 x, int32 y);

	[[nodiscard]] static Raster *convertTexToCurrentPlatform(Raster *ras);
#ifndef RWPUBLIC
	static void registerModule(void);
#endif

	enum Format {
		DEFAULT    = 0,
		C1555      = 0x0100,
		C565       = 0x0200,
		C4444      = 0x0300,
		LUM8       = 0x0400,
		C8888      = 0x0500,
		C888       = 0x0600,
		D16        = 0x0700,
		D24        = 0x0800,
		D32        = 0x0900,
		C555       = 0x0A00,
		AUTOMIPMAP = 0x1000,
		PAL8       = 0x2000,
		PAL4       = 0x4000,
		MIPMAP     = 0x8000
	};
	enum Type {
		NORMAL        = 0x00,
		ZBUFFER       = 0x01,
		CAMERA        = 0x02,
		TEXTURE       = 0x04,
		CAMERATEXTURE = 0x05,
		DONTALLOCATE  = 0x80
	};
	enum LockMode {
		LOCKWRITE	= 1,
		LOCKREAD	= 2,
		LOCKNOFETCH	= 4,	// don't fetch pixel data
		LOCKRAW		= 8,
	};

	enum
	{
		// from RW
		PRIVATELOCK_READ		= 0x02,
		PRIVATELOCK_WRITE		= 0x04,
		PRIVATELOCK_READ_PALETTE	= 0x08,
		PRIVATELOCK_WRITE_PALETTE	= 0x10,
	};
};

void conv_RGBA8888_from_RGBA8888(uint8 *out, uint8 *in);
void conv_BGRA8888_from_RGBA8888(uint8 *out, uint8 *in);
void conv_RGBA8888_from_RGB888(uint8 *out, uint8 *in);
void conv_BGRA8888_from_RGB888(uint8 *out, uint8 *in);
void conv_RGB888_from_RGB888(uint8 *out, uint8 *in);
void conv_BGR888_from_RGB888(uint8 *out, uint8 *in);
void conv_ARGB1555_from_ARGB1555(uint8 *out, uint8 *in);
void conv_ARGB1555_from_RGB555(uint8 *out, uint8 *in);
void conv_RGBA5551_from_ARGB1555(uint8 *out, uint8 *in);
void conv_ARGB1555_from_RGBA5551(uint8 *out, uint8 *in);
void conv_RGBA8888_from_ARGB1555(uint8 *out, uint8 *in);
void conv_ABGR1555_from_ARGB1555(uint8 *out, uint8 *in);
inline void conv_8_from_8(uint8 *out, uint8 *in) { *out = *in; }
// some swaps are the same, so these are just more descriptive names
inline void conv_RGBA8888_from_BGRA8888(uint8 *out, uint8 *in) { conv_BGRA8888_from_RGBA8888(out, in); }
inline void conv_RGB888_from_BGR888(uint8 *out, uint8 *in) { conv_BGR888_from_RGB888(out, in); }
inline void conv_ARGB1555_from_ABGR1555(uint8 *out, uint8 *in) { conv_ABGR1555_from_ARGB1555(out, in); }

void expandPal4(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h);
void compressPal4(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h);
void expandPal4_BE(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h);
void compressPal4_BE(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h);
void copyPal8(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h);

void flipDXT(int32 type, uint8 *dst, uint8 *src, uint32 width, uint32 height);


#define IGNORERASTERIMP 0

}

#endif // RW_RASTER_H
