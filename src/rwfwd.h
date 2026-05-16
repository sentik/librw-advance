#pragma once
#ifndef RW_FWD_H
#define RW_FWD_H

// Forward declarations for all top-level rw object types.
// Include this when you only need a pointer/reference, not the full layout.

namespace rw {

struct Object;
struct Frame;
struct FrameList_;
struct ObjectWithFrame;

struct Image;
struct ColorQuant;

struct RasterLevels;
struct Raster;

struct Texture;
struct TexDictionary;

struct SurfaceProperties;
struct Material;
struct MaterialList;
struct Mesh;
struct MeshHeader;
struct MorphTarget;
struct InstanceDataHeader;
struct Triangle;
struct Geometry;

struct Atomic;
struct Light;
struct FrustumPlane;
struct Camera;
struct Clump;
struct WorldLights;
struct World;

}

#endif // RW_FWD_H
