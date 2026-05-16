#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../rwbase.h"
#include "../rwerror.h"
#include "../rwplg.h"
#include "../rwpipeline.h"
#include "../rwscene.h"
#include "../rwgeometry.h"
#include "../rwtexture.h"
#include "../rwraster.h"
#include "../rwanim.h"
#include "../rwengine.h"
#include "../rwplugins.h"
#include "../rw/plugin/object_registry.h"
#include "rwwdgl.h"

#ifdef RW_OPENGL
#include "glad/glad.h"
#endif

#define PLUGIN_ID 2

namespace rw {
namespace wdgl {

static void*
driverOpen(void *o, int32, int32)
{
	engine->driver[PLATFORM_WDGL]->defaultPipeline = makeDefaultPipeline();
	return o;
}

static void*
driverClose(void *o, int32, int32)
{
	return o;
}

void
registerPlatformPlugins(void)
{
	Driver::registerPlugin(PLATFORM_WDGL, 0, PLATFORM_WDGL,
	                       driverOpen, driverClose);
}


// VC
//   8733 0 0 0 3
//     45 1 0 0 2
//   8657 1 3 0 2
//   4610 2 1 1 3
//   4185 3 2 1 4
//    256 4 2 1 4
//    201 4 4 1 4
//    457 5 2 0 4

// SA
//  20303 0 0 0 3	vertices:  3 floats
//     53 1 0 0 2	texCoords: 2 floats
//  20043 1 3 0 2	texCoords: 2 shorts
//   6954 2 1 1 3	normal:    3 bytes normalized
//  13527 3 2 1 4	color:     4 ubytes normalized
//    196 4 2 1 4	weight:    4 ubytes normalized
//    225 4 4 1 4	weight:    4 ushorts normalized
//    421 5 2 0 4	indices:   4 ubytes
//  12887 6 2 1 4	extracolor:4 ubytes normalized

/*
static void
printAttribInfo(AttribDesc *attribs, int n)
{
	for(int i = 0; i < n; i++)
		printf("%x %x %x %x\n",
			attribs[i].index,
			attribs[i].type,
			attribs[i].normalized,
			attribs[i].size);
}
*/

#ifdef RW_OPENGL
void
uploadGeo(Geometry *geo)
{
	InstanceDataHeader *inst = (InstanceDataHeader*)geo->instData;
	MeshHeader *meshHeader = geo->meshHeader;

	glGenBuffers(1, &inst->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, inst->vbo);
	glBufferData(GL_ARRAY_BUFFER, inst->dataSize,
	             inst->data, GL_STATIC_DRAW);

	glGenBuffers(1, &inst->ibo);
	glBindBuffer(GL_ARRAY_BUFFER, inst->ibo);
	glBufferData(GL_ARRAY_BUFFER, meshHeader->totalIndices*2,
	             0, GL_STATIC_DRAW);
	GLintptr offset = 0;
	for(uint32 i = 0; i < meshHeader->numMeshes; i++){
		Mesh *mesh = &meshHeader->getMeshes()[i];
		glBufferSubData(GL_ARRAY_BUFFER, offset, mesh->numIndices*2,
		                mesh->indices);
		offset += mesh->numIndices*2;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
setAttribPointers(InstanceDataHeader *inst)
{
	static GLenum attribType[] = {
		GL_FLOAT,
		GL_BYTE, GL_UNSIGNED_BYTE,
		GL_SHORT, GL_UNSIGNED_SHORT
	};
	for(int32 i = 0; i < inst->numAttribs; i++){
		AttribDesc *a = &inst->attribs[i];
		glEnableVertexAttribArray(a->index);
		glVertexAttribPointer(a->index, a->size, attribType[a->type],
		                      a->normalized, a->stride,
		                      (void*)(uint64)a->offset);
	}
}
#endif

void
packattrib(uint8 *dst, float32 *src, AttribDesc *a, float32 scale=1.0f)
{
	int8 *i8dst;
	uint16 *u16dst;
	int16 *i16dst;

	switch(a->type){
	case 0:	// float
		memcpy(dst, src, a->size*4);
		break;

	// TODO: maybe have loop inside if?
	case 1: // byte
		i8dst = (int8*)dst;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				i8dst[i] = src[i]*scale;
			else
				i8dst[i] = src[i]*127.0f;
		}
		break;

	case 2: // ubyte
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				dst[i] = src[i]*scale;
			else
				dst[i] = src[i]*255.0f;
		}
		break;

	case 3: // short
		i16dst = (int16*)dst;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				i16dst[i] = src[i]*scale;
			else
				i16dst[i] = src[i]*32767.0f;
		}
		break;

	case 4: // ushort
		u16dst = (uint16*)dst;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				u16dst[i] = src[i]*scale;
			else
				u16dst[i] = src[i]*65535.0f;
		}
		break;
	}
}

void
unpackattrib(float *dst, uint8 *src, AttribDesc *a, float32 scale=1.0f)
{
	int8 *i8src;
	uint16 *u16src;
	int16 *i16src;

	switch(a->type){
	case 0:	// float
		memcpy(dst, src, a->size*4);
		break;

	// TODO: maybe have loop inside if?
	case 1: // byte
		i8src = (int8*)src;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				dst[i] = i8src[i]/scale;
			else
				dst[i] = i8src[i]/127.0f;
		}
		break;

	case 2: // ubyte
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				dst[i] = src[i]/scale;
			else
				dst[i] = src[i]/255.0f;
		}
		break;

	case 3: // short
		i16src = (int16*)src;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				dst[i] = i16src[i]/scale;
			else
				dst[i] = i16src[i]/32767.0f;
		}
		break;

	case 4: // ushort
		u16src = (uint16*)src;
		for(int i = 0; i < a->size; i++){
			if(!a->normalized)
				dst[i] = u16src[i]/scale;
			else
				dst[i] = u16src[i]/65435.0f;
		}
		break;
	}
}

void*
destroyNativeData(void *object, int32, int32)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil ||
	   geometry->instData->platform != PLATFORM_WDGL)
		return object;
	InstanceDataHeader *header =
		(InstanceDataHeader*)geometry->instData;
	geometry->instData = nil;
	// TODO: delete ibo and vbo
	rwFree(header->attribs);
	rwFree(header->data);
	rwFree(header);
	return object;
}

Stream*
readNativeData(Stream *stream, int32, void *object, int32, int32)
{
	Geometry *geometry = (Geometry*)object;
	InstanceDataHeader *header = rwNewT(InstanceDataHeader, 1, MEMDUR_EVENT | ID_GEOMETRY);
	geometry->instData = header;
	header->platform = PLATFORM_WDGL;
	header->vbo = 0;
	header->ibo = 0;
	header->numAttribs = stream->readU32();
	header->attribs = rwNewT(AttribDesc, header->numAttribs, MEMDUR_EVENT | ID_GEOMETRY);
	stream->read32(header->attribs,
	            header->numAttribs*sizeof(AttribDesc));
	header->dataSize = header->attribs[0].stride*geometry->numVertices;
	header->data = rwNewT(uint8, header->dataSize, MEMDUR_EVENT | ID_GEOMETRY);
	ASSERTLITTLE;
	stream->read8(header->data, header->dataSize);
	return stream;
}

Stream*
writeNativeData(Stream *stream, int32, void *object, int32, int32)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil ||
	   geometry->instData->platform != PLATFORM_WDGL)
		return stream;
	InstanceDataHeader *header = (InstanceDataHeader*)geometry->instData;
	stream->writeU32(header->numAttribs);
	stream->write32(header->attribs, header->numAttribs*sizeof(AttribDesc));
	ASSERTLITTLE;
	stream->write8(header->data, header->dataSize);
	return stream;
}

int32
getSizeNativeData(void *object, int32, int32)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil ||
	   geometry->instData->platform != PLATFORM_WDGL)
		return 0;
	InstanceDataHeader *header = (InstanceDataHeader*)geometry->instData;
	return 4 + header->numAttribs*sizeof(AttribDesc) + header->dataSize;
}

void
registerNativeDataPlugin(void)
{
	using namespace rw::plugin;
	auto& reg = ObjectRegistry<Geometry>::instance();
	(void)reg.registerExtension<uint8_t>(fromRaw(ID_NATIVEDATA),
		PluginLifecycle{
			.destruct = [](void* o, std::ptrdiff_t) {
				destroyNativeData(o, 0, 0);
			},
		},
		PluginStream{
			.read = [](rw::Stream& stream, std::int32_t len, void* o, std::ptrdiff_t)
				-> std::expected<void, StreamPluginError> {
				if(readNativeData(&stream, len, o, 0, 0)) return {};
				return std::unexpected(StreamPluginError::callbackFailed);
			},
			.write = [](rw::Stream& stream, std::int32_t len, const void* o, std::ptrdiff_t)
				-> std::expected<void, StreamPluginError> {
				writeNativeData(&stream, len, const_cast<void*>(o), 0, 0);
				return {};
			},
			.getSize = [](const void* o, std::ptrdiff_t) -> std::int32_t {
				return getSizeNativeData(const_cast<void*>(o), 0, 0);
			},
		},
		"nativedata-wdgl");
}

void
printPipeinfo(Atomic *a)
{
	Geometry *g = a->geometry;
	if(g->instData == nil || g->instData->platform != PLATFORM_WDGL)
		return;
	int32 plgid = 0;
	if(a->pipeline)
		plgid = a->pipeline->pluginID;
	printf("%s %x: ", debugFile, plgid);
	InstanceDataHeader *h = (InstanceDataHeader*)g->instData;
	for(int i = 0; i < h->numAttribs; i++)
		printf("%x(%x) ", h->attribs[i].index, h->attribs[i].type);
	printf("\n");
}

static void
instance(rw::ObjPipeline *rwpipe, Atomic *atomic)
{
	ObjPipeline *pipe = (ObjPipeline*)rwpipe;
	Geometry *geo = atomic->geometry;
	// TODO: allow for REINSTANCE (or not, wdgl can't render)
	if(geo->instData)
		return;
	InstanceDataHeader *header = rwNewT(InstanceDataHeader, 1, MEMDUR_EVENT | ID_GEOMETRY);
	geo->instData = header;
	header->platform = PLATFORM_WDGL;
	header->vbo = 0;
	header->ibo = 0;
	header->numAttribs =
		pipe->numCustomAttribs + 1 + (geo->numTexCoordSets > 0);
	if(geo->flags & Geometry::PRELIT)
		header->numAttribs++;
	if(geo->flags & Geometry::NORMALS)
		header->numAttribs++;
	int32 offset = 0;
	header->attribs = rwNewT(AttribDesc, header->numAttribs, MEMDUR_EVENT | ID_GEOMETRY);

	AttribDesc *a = header->attribs;
	// Vertices
	a->index = 0;
	a->type = 0;
	a->normalized = 0;
	a->size = 3;
	a->offset = offset;
	offset += 12;
	a++;
	int32 firstCustom = 1;

	// texCoords, only one set here
	if(geo->numTexCoordSets){
		a->index = 1;
		a->type = 3;
		a->normalized = 0;
		a->size = 2;
		a->offset = offset;
		offset += 4;
		a++;
		firstCustom++;
	}

	if(geo->flags & Geometry::NORMALS){
		a->index = 2;
		a->type = 1;
		a->normalized = 1;
		a->size = 3;
		a->offset = offset;
		offset += 4;
		a++;
		firstCustom++;
	}

	if(geo->flags & Geometry::PRELIT){
		a->index = 3;
		a->type = 2;
		a->normalized = 1;
		a->size = 4;
		a->offset = offset;
		offset += 4;
		a++;
		firstCustom++;
	}

	if(pipe->instanceCB)
		offset += pipe->instanceCB(geo, firstCustom, offset);
	else{
		header->dataSize = offset*geo->numVertices;
		header->data = rwNewT(uint8, header->dataSize, MEMDUR_EVENT | ID_GEOMETRY);
	}

	a = header->attribs;
	for(int32 i = 0; i < header->numAttribs; i++)
		a[i].stride = offset;

	uint8 *p = header->data + a->offset;
	V3d *vert = geo->morphTargets->vertices;
	for(int32 i = 0; i < geo->numVertices; i++){
		packattrib(p, (float32*)vert, a);
		vert++;
		p += a->stride;
	}
	a++;

	if(geo->numTexCoordSets){
		p = header->data + a->offset;
		TexCoords *texcoord = geo->texCoords[0];
		for(int32 i = 0; i < geo->numVertices; i++){
			packattrib(p, (float32*)texcoord, a, 512.0f);
			texcoord++;
			p += a->stride;
		}
		a++;
	}

	if(geo->flags & Geometry::NORMALS){
		p = header->data + a->offset;
		V3d *norm = geo->morphTargets->normals;
		for(int32 i = 0; i < geo->numVertices; i++){
			packattrib(p, (float32*)norm, a);
			norm++;
			p += a->stride;
		}
		a++;
	}

	if(geo->flags & Geometry::PRELIT){
		// TODO: this seems too complicated
		p = header->data + a->offset;
		RGBA *color = geo->colors;
		float32 f[4];
		for(int32 i = 0; i < geo->numVertices; i++){
			f[0] = color->red/255.0f;
			f[1] = color->green/255.0f;
			f[2] = color->blue/255.0f;
			f[3] = color->alpha/255.0f;
			packattrib(p, (float32*)f, a);
			color++;
			p += a->stride;
		}
		a++;
	}
}

static void
uninstance(rw::ObjPipeline *rwpipe, Atomic *atomic)
{
	ObjPipeline *pipe = (ObjPipeline*)rwpipe;
	Geometry *geo = atomic->geometry;
	if((geo->flags & Geometry::NATIVE) == 0)
		return;
	assert(geo->instData != nil);
	assert(geo->instData->platform == PLATFORM_WDGL);
	geo->numTriangles = geo->meshHeader->guessNumTriangles();
	geo->allocateData();

	uint8 *p;
	TexCoords *texcoord = geo->texCoords[0];
	RGBA *color = geo->colors;
	V3d *vert = geo->morphTargets->vertices;
	V3d *norm = geo->morphTargets->normals;
	float32 f[4];

	InstanceDataHeader *header = (InstanceDataHeader*)geo->instData;
	for(int i = 0; i < header->numAttribs; i++){
		AttribDesc *a = &header->attribs[i];
		p = header->data + a->offset;

		switch(a->index){
		case 0:		// Vertices
			for(int32 i = 0; i < geo->numVertices; i++){
				unpackattrib((float32*)vert, p, a);
				vert++;
				p += a->stride;
			}
			break;

		case 1:		// texCoords
			for(int32 i = 0; i < geo->numVertices; i++){
				unpackattrib((float32*)texcoord, p, a, 512.0f);
				texcoord++;
				p += a->stride;
			}
			break;

		case 2:		// normals
			for(int32 i = 0; i < geo->numVertices; i++){
				unpackattrib((float32*)norm, p, a);
				norm++;
				p += a->stride;
			}
			break;

		case 3:		// colors
			for(int32 i = 0; i < geo->numVertices; i++){
				// TODO: this seems too complicated
				unpackattrib(f, p, a);
				color->red   = f[0]*255.0f;
				color->green = f[1]*255.0f;
				color->blue  = f[2]*255.0f;
				color->alpha = f[3]*255.0f;
				color++;
				p += a->stride;
			}
			break;
		}
	}

	if(pipe->uninstanceCB)
		pipe->uninstanceCB(geo);

	geo->generateTriangles();

	geo->flags &= ~Geometry::NATIVE;
	destroyNativeData(geo, 0, 0);
}

void
ObjPipeline::init(void)
{
	this->rw::ObjPipeline::init(PLATFORM_GL3);
	this->numCustomAttribs = 0;
	this->impl.instance = wdgl::instance;
	this->impl.uninstance = wdgl::uninstance;
	this->instanceCB = nil;
	this->uninstanceCB = nil;
}

ObjPipeline*
ObjPipeline::create(void)
{
	ObjPipeline *pipe = rwNewT(ObjPipeline, 1, MEMDUR_GLOBAL);
	pipe->init();
	return pipe;
}

ObjPipeline*
makeDefaultPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	return pipe;
}

// Skin

Stream*
readNativeSkin(Stream *stream, int32, void *object, int32 offset)
{
	Geometry *geometry = (Geometry*)object;
	uint32 platform;
	if(!findChunk(stream, ID_STRUCT, nil, nil)){
		RWERROR((ERR_CHUNK, "STRUCT"));
		return nil;
	}
	platform = stream->readU32();
	if(platform != PLATFORM_GL){
		RWERROR((ERR_PLATFORM, platform));
		return nil;
	}
	Skin *skin = rwNewT(Skin, 1, MEMDUR_EVENT | ID_SKIN);
	*(Skin**)((uint8*)geometry + offset) = skin;

	int32 numBones = stream->readI32();
	skin->init(numBones, 0, 0);
	stream->read32(skin->inverseMatrices, skin->numBones*64);
	return stream;
}

Stream*
writeNativeSkin(Stream *stream, int32 len, void *object, int32 offset)
{
	writeChunkHeader(stream, ID_STRUCT, len-12);
	stream->writeU32(PLATFORM_GL);
	Skin *skin = *(Skin**)((uint8*)object + offset);
	stream->writeI32(skin->numBones);
	stream->write32(skin->inverseMatrices, skin->numBones*64);
	return stream;
}

int32
getSizeNativeSkin(void *object, int32 offset)
{
	Skin *skin = *(Skin**)((uint8*)object + offset);
	if(skin == nil)
		return -1;
	int32 size = 12 + 4 + 4 + skin->numBones*64;
	return size;
}

uint32
skinInstanceCB(Geometry *g, int32 i, uint32 offset)
{
	InstanceDataHeader *header = (InstanceDataHeader*)g->instData;
	AttribDesc *a = &header->attribs[i];
	// weights
	a->index = 4;
	a->type = 2;	/* but also short o_O */
	a->normalized = 1;
	a->size = 4;
	a->offset = offset;
	offset += 4;
	a++;

	// indices
	a->index = 5;
	a->type = 2;
	a->normalized = 0;
	a->size = 4;
	a->offset = offset;
	offset += 4;

	header->dataSize = offset*g->numVertices;
	header->data = rwNewT(uint8, header->dataSize, MEMDUR_EVENT | ID_GEOMETRY);

	Skin *skin = Skin::get(g);
	if(skin == nil)
		return 8;

	a = &header->attribs[i];
	uint8 *wgt = header->data + a[0].offset;
	uint8 *idx = header->data + a[1].offset;
	uint8 *indices = skin->indices;
	float32 *weights = skin->weights;
	for(int32 i = 0; i < g->numVertices; i++){
		packattrib(wgt, weights, a);
		weights += 4;
		wgt += offset;
		idx[0] = *indices++;
		idx[1] = *indices++;
		idx[2] = *indices++;
		idx[3] = *indices++;
		idx += offset;
	}

	return 8;
}

void
skinUninstanceCB(Geometry *geo)
{
	InstanceDataHeader *header = (InstanceDataHeader*)geo->instData;

	Skin *skin = Skin::get(geo);
	if(skin == nil)
		return;

	uint8 *data = skin->data;
	float *invMats = skin->inverseMatrices;
	skin->init(skin->numBones, skin->numBones, geo->numVertices);
	memcpy(skin->inverseMatrices, invMats, skin->numBones*64);
	rwFree(data);

	uint8 *p;
	float *weights = skin->weights;
	uint8 *indices = skin->indices;
	for(int i = 0; i < header->numAttribs; i++){
		AttribDesc *a = &header->attribs[i];
		p = header->data + a->offset;

		switch(a->index){
		case 4:		// weights
			for(int32 i = 0; i < geo->numVertices; i++){
				unpackattrib(weights, p, a);
float sum = weights[0] + weights[1] + weights[2] + weights[3];
if(sum){
	weights[0] /= sum;
	weights[1] /= sum;
	weights[2] /= sum;
	weights[3] /= sum;
}
				weights += 4;
				p += a->stride;
			}
			break;

		case 5:		// indices
			for(int32 i = 0; i < geo->numVertices; i++){
				*indices++ = p[0];
				*indices++ = p[1];
				*indices++ = p[2];
				*indices++ = p[3];
				p += a->stride;
			}
			break;
		}
	}

	skin->findNumWeights(geo->numVertices);
	skin->findUsedBones(geo->numVertices);
}

// Skin

static void*
skinOpen(void *o, int32, int32)
{
	skinGlobals.pipelines[PLATFORM_WDGL] = makeSkinPipeline();
	return o;
}

static void*
skinClose(void *o, int32, int32)
{
	((ObjPipeline*)skinGlobals.pipelines[PLATFORM_WDGL])->destroy();
	skinGlobals.pipelines[PLATFORM_WDGL] = nil;
	return o;
}

void
initSkin(void)
{
	Driver::registerPlugin(PLATFORM_WDGL, 0, ID_SKIN,
	                       skinOpen, skinClose);
}

ObjPipeline*
makeSkinPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	pipe->pluginID = ID_SKIN;
	pipe->pluginData = 1;
	pipe->numCustomAttribs = 2;
	pipe->instanceCB = skinInstanceCB;
	pipe->uninstanceCB = skinUninstanceCB;
	return pipe;
}

// MatFX

static void*
matfxOpen(void *o, int32, int32)
{
	matFXGlobals.pipelines[PLATFORM_WDGL] = makeMatFXPipeline();
	return o;
}

static void*
matfxClose(void *o, int32, int32)
{
	((ObjPipeline*)matFXGlobals.pipelines[PLATFORM_WDGL])->destroy();
	matFXGlobals.pipelines[PLATFORM_WDGL] = nil;
	return o;
}

void
initMatFX(void)
{
	Driver::registerPlugin(PLATFORM_WDGL, 0, ID_MATFX,
	                       matfxOpen, matfxClose);
}

ObjPipeline*
makeMatFXPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	pipe->pluginID = ID_MATFX;
	pipe->pluginData = 0;
	return pipe;
}

// Raster

int32 nativeRasterOffset;

#ifdef RW_OPENGL
struct GlRaster {
	GLuint id;
};

static void*
createNativeRaster(void *object, std::ptrdiff_t offset, int32)
{
	GlRaster *raster = (GlRaster*)((uint8*)object + offset);
	raster->id = 0;
	return object;
}

static void*
destroyNativeRaster(void *object, std::ptrdiff_t offset, int32)
{
	// TODO:
	return object;
}

static void*
copyNativeRaster(void *dst, void *, std::ptrdiff_t offset, int32)
{
	GlRaster *raster = (GlRaster*)((uint8*)dst + offset);
	raster->id = 0;
	return dst;
}

void
registerNativeRaster(void)
{
	using namespace rw::plugin;
	auto& reg = ObjectRegistry<Raster>::instance();
	auto result = reg.registerExtension<GlRaster>(fromRaw(ID_RASTERWDGL),
		PluginLifecycle{
			.construct = [](void* o, std::ptrdiff_t off) {
				createNativeRaster(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyNativeRaster(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyNativeRaster(d, const_cast<void*>(s), off, 0);
			},
		},
		PluginStream{},
		"nativeraster-wdgl");
	if(result)
		nativeRasterOffset = static_cast<int32>(result->value());
}

void
Texture::upload(void)
{
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	Raster *r = this->raster;
	if(r->palette){
		printf("can't upload paletted raster\n");
		return;
	}

	static GLenum filter[] = {
		0, GL_NEAREST, GL_LINEAR,
		   GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
		   GL_NEAREST_MIPMAP_LINEAR,  GL_LINEAR_MIPMAP_LINEAR
	};
	static GLenum filternomip[] = {
		0, GL_NEAREST, GL_LINEAR,
		   GL_NEAREST, GL_LINEAR,
		   GL_NEAREST, GL_LINEAR
	};
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		filternomip[this->filterAddressing & 0xFF]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		filternomip[this->filterAddressing & 0xFF]);

	static GLenum wrap[] = {
		0, GL_REPEAT, GL_MIRRORED_REPEAT,
		GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
	};
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
		wrap[(this->filterAddressing >> 8) & 0xF]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
		wrap[(this->filterAddressing >> 12) & 0xF]);

	switch(r->format & 0xF00){
	case Raster::C8888:
		glTexImage2D(GL_TEXTURE_2D, 0, 4, r->width, r->height,
		             0, GL_RGBA, GL_UNSIGNED_BYTE, r->pixels);
		break;
	default:
		printf("unsupported raster format: %x\n", r->format);
		break;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	GlRaster *glr = (GlRaster*)((uint8*)r + nativeRasterOffset);
	glr->id = id;
}

void
Texture::bind(int n)
{
	Raster *r = this->raster;
	GlRaster *glr = (GlRaster*)((uint8*)r + nativeRasterOffset);
	glActiveTexture(GL_TEXTURE0+n);
	if(r){
		if(glr->id == 0)
			this->upload();
		glBindTexture(GL_TEXTURE_2D, glr->id);
	}else
		glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

}
#endif

}
}
