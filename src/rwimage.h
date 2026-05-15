#pragma once
#ifndef RW_IMAGE_H
#define RW_IMAGE_H

#include "rwbase.h"

namespace rw {

struct Image
{
	int32 flags;
	int32 width, height;
	int32 depth;
	int32 bpp;	// bytes per pixel
	int32 stride;
	uint8 *pixels;
	uint8 *palette;

	static int32 numAllocated;

	static Image *create(int32 width, int32 height, int32 depth);
	void destroy(void);
	void allocate(void);
	void free(void);
	void setPixels(uint8 *pixels);
	void setPixelsDXT(int32 type, uint8 *pixels);
	void setPalette(uint8 *palette);
	void compressPalette(void);	// turn 8 bit into 4 bit if possible
	bool32 hasAlpha(void);
	void convertTo32(void);
	void palettize(int32 depth);
	void unpalettize(bool forceAlpha = false);
	void makeMask(void);
	void applyMask(Image *mask);
	void removeMask(void);
	Image *extractMask(void);

	static void setSearchPath(const char*);
	static void printSearchPath(void);
	static char *getFilename(const char*);
	static Image *read(const char *imageName);
	static Image *readMasked(const char *imageName, const char *maskName);


	using fileRead = Image*(*)(const char *afilename);
	using fileWrite = void(*)(Image *image, const char *filename);
	static bool32 registerFileFormat(const char *ext, fileRead read, fileWrite write);

#ifndef RWPUBLIC
	static void registerModule(void);
#endif
};

Image *readTGA(const char *filename);
void writeTGA(Image *image, const char *filename);
Image *readBMP(const char *filename);
void writeBMP(Image *image, const char *filename);
Image *readPNG(const char *filename);
void writePNG(Image *image, const char *filename);

enum { QUANTDEPTH = 8 };

struct ColorQuant
{
	struct Node {
		uint32 r, g, b, a;
		int32 numPixels;
		Node *parent;
		Node *children[16];
		LLLink link;

		void destroy(void);
		void addColor(RGBA color);
		bool isLeaf(void) { for(int32 i = 0; i < 16; i++) if(this->children[i]) return false; return true; }
	};

	Node *root;
	LinkList leaves;

	void init(void);
	void destroy(void);
	Node *createNode(int32 level);
	Node *getNode(Node *root, uint32 addr, int32 level);
	Node *findNode(Node *root, uint32 addr, int32 level);
	void reduceNode(Node *node);
	void addColor(RGBA color);
	uint8 findColor(RGBA color);
	void addImage(Image *img);
	void makePalette(int32 numColors, RGBA *colors);
	void matchImage(uint8 *dstPixels, uint32 dstStride, Image *src);
};

}

#endif // RW_IMAGE_H
