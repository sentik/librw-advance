#pragma once
#ifndef RW_GEOMETRY_H
#define RW_GEOMETRY_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwpipeline.h"
#include "rwobject.h"
#include "rwfwd.h"

namespace rw {

struct SurfaceProperties
{
	float32 ambient;
	float32 specular;
	float32 diffuse;
};

struct Material
{
	PLUGINBASE
	Texture *texture;
	RGBA color;
	SurfaceProperties surfaceProps;
	Pipeline *pipeline;
	int32 refCount;

	static int32 numAllocated;

	[[nodiscard]] static Material *create(void);
	void addRef(void) noexcept { this->refCount++; }
	[[nodiscard]] Material *clone(void);
	void destroy(void);
	void setTexture(Texture *tex);
	static Material *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);
};

void registerMaterialRightsPlugin(void);

struct Mesh
{
	uint16 *indices;
	uint32 numIndices;
	Material *material;
};

struct MeshHeader
{
	enum {
		TRISTRIP = 1
	};
	uint32 flags;
	uint16 numMeshes;
	uint16 serialNum;
	uint32 totalIndices;
	uint32 pad;	// needed for alignment of Meshes
	// after this the meshes

	[[nodiscard]] Mesh *getMeshes(void) noexcept { return (Mesh*)(this+1); }
	void setupIndices(void);
	[[nodiscard]] uint32 guessNumTriangles(void);
};

struct MorphTarget
{
	Geometry *parent;
	Sphere boundingSphere;
	V3d *vertices;
	V3d *normals;

	[[nodiscard]] Sphere calculateBoundingSphere(void) const;
};

struct InstanceDataHeader
{
	uint32 platform;
};

struct Triangle
{
	uint16 v[3];
	uint16 matId;
};

struct MaterialList
{
	Material **materials;
	int32 numMaterials;
	int32 space;

	void init(void);
	void deinit(void);
	[[nodiscard]] int32 appendMaterial(Material *mat);
	[[nodiscard]] int32 findIndex(Material *mat);
	static MaterialList *streamRead(Stream *stream, MaterialList *matlist);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);
};

struct Geometry
{
	PLUGINBASE
	enum { ID = 8 };
	Object object;
	uint32 flags;
	uint16 lockedSinceInst;
	int32 numTriangles;
	int32 numVertices;
	int32 numMorphTargets;
	int32 numTexCoordSets;

	Triangle *triangles;
	RGBA *colors;
	TexCoords *texCoords[8];

	MorphTarget *morphTargets;
	MaterialList matList;

	MeshHeader *meshHeader;
	InstanceDataHeader *instData;

	int32 refCount;

	static int32 numAllocated;

	[[nodiscard]] static Geometry *create(int32 numVerts, int32 numTris, uint32 flags);
	void addRef(void) noexcept { this->refCount++; }
	void destroy(void);
	void lock(int32 lockFlags);
	void unlock(void);
	void addMorphTargets(int32 n);
	void calculateBoundingSphere(void);
	[[nodiscard]] bool32 hasColoredMaterial(void);
	void allocateData(void);
	MeshHeader *allocateMeshes(int32 numMeshes, uint32 numIndices, bool32 noIndices);
	void generateTriangles(int8 *adc = nullptr);
	void buildMeshes(void);
	void buildTristrips(void);	// private, used by buildMeshes
	void correctTristripWinding(void);
	void removeUnusedMaterials(void);
	[[nodiscard]] static Geometry *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);

	enum Flags
	{
		TRISTRIP  = 0x01,
		POSITIONS = 0x02,
		TEXTURED  = 0x04,
		PRELIT    = 0x08,
		NORMALS   = 0x10,
		LIGHT     = 0x20,
		MODULATE  = 0x40,
		TEXTURED2 = 0x80,
		// When this flag is set the geometry has
		// native geometry. When streamed out this geometry
		// is written out instead of the platform independent data.
		// When streamed in with this flag, the geometry is mostly empty.
		NATIVE         = 0x01000000,
		// Just for documentation: RW sets this flag
		// to prevent rendering when executing a pipeline,
		// so only instancing will occur.
		// librw's pipelines are different so it's unused here.
		NATIVEINSTANCE = 0x02000000
	};

	enum LockFlags
	{
		LOCKPOLYGONS     = 0x0001,
		LOCKVERTICES     = 0x0002,
		LOCKNORMALS      = 0x0004,
		LOCKPRELIGHT     = 0x0008,

		LOCKTEXCOORDS    = 0x0010,
		LOCKTEXCOORDS1   = 0x0010,
		LOCKTEXCOORDS2   = 0x0020,
		LOCKTEXCOORDS3   = 0x0040,
		LOCKTEXCOORDS4   = 0x0080,
		LOCKTEXCOORDS5   = 0x0100,
		LOCKTEXCOORDS6   = 0x0200,
		LOCKTEXCOORDS7   = 0x0400,
		LOCKTEXCOORDS8   = 0x0800,
		LOCKTEXCOORDSALL = 0x0ff0,

		LOCKALL          = 0x0fff
	};
};

void registerMeshPlugin(void);
void registerNativeDataPlugin(void);

}

#endif // RW_GEOMETRY_H
