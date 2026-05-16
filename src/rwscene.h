#pragma once
#ifndef RW_SCENE_H
#define RW_SCENE_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwpipeline.h"
#include "rwobject.h"
#include "rwframe.h"
#include "rwfwd.h"
#include <array>
#include <functional>
#include <type_traits>

namespace rw {

struct Atomic
{
	PLUGINBASE
	using RenderCB = void(*)(Atomic *atomic);
	static inline constexpr int32 ID = 1;
	enum {
	// flags
		COLLISIONTEST = 0x01,	// unused here
		RENDER = 0x04,
	// private flags
		WORLDBOUNDDIRTY = 0x01,
	// for setGeometry
		SAMEBOUNDINGSPHERE = 0x01
	};

	ObjectWithFrame object;
	Geometry *geometry;
	Sphere boundingSphere;
	Sphere worldBoundingSphere;
	Clump *clump;
	LLLink inClump;
	ObjPipeline *pipeline;
	RenderCB renderCB;

	World *world;
	ObjectWithFrame::Sync originalSync;

	static int32 numAllocated;

	[[nodiscard]] static Atomic *create(void);
	[[nodiscard]] Atomic *clone(void);
	void destroy(void);
	void setFrame(Frame *f) {
		this->object.setFrame(f);
		this->object.object.privateFlags |= WORLDBOUNDDIRTY;
	}
	[[nodiscard]] Frame *getFrame(void) const noexcept { return (Frame*)this->object.object.parent; }
	[[nodiscard]] static Atomic *fromClump(LLLink *lnk){
		return LLLinkGetData(lnk, Atomic, inClump); }
	void setGeometry(Geometry *geo, uint32 flags);
	[[nodiscard]] Sphere *getWorldBoundingSphere(void);
	[[nodiscard]] ObjPipeline *getPipeline(void);
	void instance(void);
	void uninstance(void);
	void render(void) { this->renderCB(this); }
	void setRenderCB(RenderCB renderCB){
		this->renderCB = renderCB ? renderCB : defaultRenderCB;
	}
	void setRenderCB(std::function<void(Atomic*)> fn){
		if(auto *ptr = fn.template target<RenderCB>())
			setRenderCB(*ptr);
		else
			setRenderCB(fn ? defaultRenderCB : nullptr);
	}
	void setFlags(uint32 flags) noexcept { this->object.object.flags = flags; }
	[[nodiscard]] uint32 getFlags(void) const noexcept { return this->object.object.flags; }
	[[nodiscard]] static Atomic *streamReadClump(Stream *stream,
		FrameList_ *frameList, Geometry **geometryList);
	bool streamWriteClump(Stream *stream, FrameList_ *frmlst);
	uint32 streamGetSize(void);

	static void defaultRenderCB(Atomic *atomic);
};

void registerAtomicRightsPlugin(void);

struct Light
{
	PLUGINBASE
	using plugin_owner_tag = Light;
	static inline constexpr int32 ID = 3;
	ObjectWithFrame object;
	float32 radius;
	RGBAf color;
	float32 minusCosAngle;
	LLLink inWorld;

	// clump extension
	Clump *clump;
	LLLink inClump;

	// world extension
	World *world;
	ObjectWithFrame::Sync originalSync;

	static int32 numAllocated;

	[[nodiscard]] static Light *create(int32 type);
	void destroy(void);
	void setFrame(Frame *f) { this->object.setFrame(f); }
	[[nodiscard]] Frame *getFrame(void) const noexcept { return (Frame*)this->object.object.parent; }
	[[nodiscard]] static Light *fromClump(LLLink *lnk){
		return LLLinkGetData(lnk, Light, inClump); }
	[[nodiscard]] static Light *fromWorld(LLLink *lnk){
		return LLLinkGetData(lnk, Light, inWorld); }
	void setAngle(float32 angle);
	[[nodiscard]] float32 getAngle(void);
	void setColor(float32 r, float32 g, float32 b);
	[[nodiscard]] int32 getType(void) const noexcept { return this->object.object.subType; }
	void setFlags(uint32 flags) noexcept { this->object.object.flags = flags; }
	[[nodiscard]] uint32 getFlags(void) const noexcept { return this->object.object.flags; }
	[[nodiscard]] static Light *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);

	enum Type {
		DIRECTIONAL = 1,
		AMBIENT,
		POINT = 0x80,	// positioned
		SPOT,
		SOFTSPOT
	};
	enum Flags {
		LIGHTATOMICS = 1,
		LIGHTWORLD = 2
	};
};

struct FrustumPlane
{
	Plane plane;
	/* Used for BBox tests:
	 * 0 = inf is closer to normal direction
	 * 1 = sup is closer to normal direction */
	uint8 closestX;
	uint8 closestY;
	uint8 closestZ;
};

struct Camera
{
	PLUGINBASE
	using plugin_owner_tag = Camera;
	static inline constexpr int32 ID = 4;
	enum class Projection : int32 { Perspective = 1, Parallel };
	enum : uint32 { CLEARIMAGE = 0x1, CLEARZ = 0x2, CLEARSTENCIL = 0x4 };
	enum class FrustumResult : int32 { Outside = 0, Boundary, Inside };

	ObjectWithFrame object;
	void (*beginUpdateCB)(Camera*);
	void (*endUpdateCB)(Camera*);
	V2d viewWindow;
	V2d viewOffset;
	float32 nearPlane, farPlane;
	float32 fogPlane;
	Projection projection;

	Matrix viewMatrix;
	float32 zScale, zShift;

	std::array<FrustumPlane, 6> frustumPlanes;
	std::array<V3d, 8> frustumCorners;
	BBox frustumBoundBox;

	Raster *frameBuffer;
	Raster *zBuffer;

	// Device dependent view and projection matrices
	// optional
	RawMatrix devView;
	RawMatrix devProj;

	// clump link handled by plugin in RW
	Clump *clump;
	LLLink inClump;

	// world extension
	/* RW: frustum sectors, space, position */
	World *world;
	ObjectWithFrame::Sync originalSync;
	void (*originalBeginUpdate)(Camera*);
	void (*originalEndUpdate)(Camera*);

	static int32 numAllocated;

	[[nodiscard]] static Camera *create(void);
	[[nodiscard]] Camera *clone(void);
	void destroy(void);
	void setFrame(Frame *f) { this->object.setFrame(f); }
	[[nodiscard]] Frame *getFrame(void) const noexcept { return (Frame*)this->object.object.parent; }
	[[nodiscard]] static Camera *fromClump(LLLink *lnk){
		return LLLinkGetData(lnk, Camera, inClump); }
	void beginUpdate(void) { this->beginUpdateCB(this); }
	void endUpdate(void) { this->endUpdateCB(this); }

	using UpdateCB = void(*)(Camera*);
	void setBeginUpdateCB(UpdateCB cb) noexcept { this->beginUpdateCB = cb; }
	void setEndUpdateCB(UpdateCB cb) noexcept   { this->endUpdateCB   = cb; }
	void setBeginUpdateCB(std::function<void(Camera*)> fn){
		if(auto *ptr = fn.template target<UpdateCB>()) setBeginUpdateCB(*ptr);
	}
	void setEndUpdateCB(std::function<void(Camera*)> fn){
		if(auto *ptr = fn.template target<UpdateCB>()) setEndUpdateCB(*ptr);
	}

	void clear(RGBA *col, uint32 mode);
	void showRaster(uint32 flags);
	void setNearPlane(float32);
	void setFarPlane(float32);
	void setViewWindow(const V2d *window);
	void setViewOffset(const V2d *offset);
	void setProjection(Projection proj);
	[[nodiscard]] FrustumResult frustumTestSphere(const Sphere *s) const;
	static Camera *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);

	// fov in degrees
	void setFOV(float32 fov, float32 ratio);
};

struct Clump
{
	PLUGINBASE
	static inline constexpr int32 ID = 2;
	Object object;
	LinkList atomics;
	LinkList lights;
	LinkList cameras;

	World *world;
	LLLink inWorld;

	static int32 numAllocated;

	[[nodiscard]] static Clump *create(void);
	[[nodiscard]] Clump *clone(void);
	void destroy(void);
	[[nodiscard]] static Clump *fromWorld(LLLink *lnk){
		return LLLinkGetData(lnk, Clump, inWorld); }
	[[nodiscard]] int32 countAtomics(void) { return this->atomics.count(); }
	void addAtomic(Atomic *a);
	void removeAtomic(Atomic *a);
	[[nodiscard]] int32 countLights(void) { return this->lights.count(); }
	void addLight(Light *l);
	void removeLight(Light *l);
	[[nodiscard]] int32 countCameras(void) { return this->cameras.count(); }
	void addCamera(Camera *c);
	void removeCamera(Camera *c);
	void setFrame(Frame *f) noexcept {
		this->object.parent = f; }
	[[nodiscard]] Frame *getFrame(void) const noexcept {
		return (Frame*)this->object.parent; }
	[[nodiscard]] static Clump *streamRead(Stream *stream);
	bool streamWrite(Stream *stream);
	uint32 streamGetSize(void);
	void render(void);
};

// used by enumerateLights for lighting callback
struct WorldLights
{
	int32 numAmbients;
	RGBAf ambient;	// all ambients added
	int32 numDirectionals;
	Light **directionals;	// only directionals
	int32 numLocals;
	Light **locals;	// points, (soft)spots
};

// A bit of a stub right now
struct World
{
	PLUGINBASE
	static inline constexpr int32 ID = 7;
	Object object;
	LinkList localLights;	// these have positions (type >= 0x80)
	LinkList globalLights;	// these do not (type < 0x80)
	LinkList clumps;

	static int32 numAllocated;

	[[nodiscard]] static World *create(BBox *bbox = nullptr);	// TODO: should probably make this non-optional
	void destroy(void);
	void addLight(Light *light);
	void removeLight(Light *light);
	void addCamera(Camera *cam);
	void removeCamera(Camera *cam);
	void addAtomic(Atomic *atomic);
	void removeAtomic(Atomic *atomic);
	void addClump(Clump *clump);
	void removeClump(Clump *clump);
	void render(void);
	void enumerateLights(Atomic *atomic, WorldLights *lightData);
	void enumerateLights(WorldLights *lightData);
};

static_assert(std::is_standard_layout_v<Atomic>,  "Atomic must be standard-layout for plugin offsets");
static_assert(std::is_standard_layout_v<Light>,   "Light must be standard-layout for plugin offsets");
static_assert(std::is_standard_layout_v<Camera>,  "Camera must be standard-layout for plugin offsets");
static_assert(std::is_standard_layout_v<Clump>,   "Clump must be standard-layout for plugin offsets");
static_assert(std::is_standard_layout_v<World>,   "World must be standard-layout for plugin offsets");

}

#endif // RW_SCENE_H
