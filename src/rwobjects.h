#pragma once
#ifndef RW_OBJECTS_H
#define RW_OBJECTS_H

// Umbrella header. Historically all object types lived in this single file;
// they are now split across domain-focused headers. Prefer including the
// granular header that matches your usage:
//
//   rwobject.h    Object
//   rwframe.h     Frame, FrameList_, ObjectWithFrame
//   rwimage.h     Image, ColorQuant, image-format I/O
//   rwraster.h    Raster, RasterLevels, pixel conversion helpers
//   rwtexture.h   Texture, TexDictionary, anisotropy plugin
//   rwgeometry.h  Material, MaterialList, Mesh, MeshHeader, Triangle,
//                 MorphTarget, InstanceDataHeader, Geometry
//   rwscene.h     Atomic, Light, FrustumPlane, Camera, Clump,
//                 WorldLights, World
//
// rwfwd.h provides forward declarations of every object type.

#include <stddef.h>

#include "rwobject.h"
#include "rwframe.h"
#include "rwimage.h"
#include "rwraster.h"
#include "rwtexture.h"
#include "rwgeometry.h"
#include "rwscene.h"

#endif // RW_OBJECTS_H
