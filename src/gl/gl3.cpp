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
#include "../rwimage.h"
#include "../rwengine.h"

#include "rwgl3.h"
#include "rwgl3shader.h"

#include "rwgl3impl.h"
#include "../rw/plugin/driver_platform_registry.h"

namespace rw {
namespace gl3 {

// TODO: make some of these things platform-independent

void
registerPlatformPlugins(void)
{
	using namespace rw::plugin;
	(void)DriverPlatformRegistry::instance().registerPlatformLifecycle(
	    PLATFORM_GL3, fromRaw(PLATFORM_GL3),
	    PluginLifecycle{
	        .construct = [](void*, std::ptrdiff_t) {
#ifdef RW_OPENGL
	            engine->driver[PLATFORM_GL3]->defaultPipeline = makeDefaultPipeline();
#endif
	            engine->driver[PLATFORM_GL3]->rasterNativeOffset = nativeRasterOffset.value();
	            engine->driver[PLATFORM_GL3]->rasterCreate       = rasterCreate;
	            engine->driver[PLATFORM_GL3]->rasterLock         = rasterLock;
	            engine->driver[PLATFORM_GL3]->rasterUnlock       = rasterUnlock;
	            engine->driver[PLATFORM_GL3]->rasterNumLevels    = rasterNumLevels;
	            engine->driver[PLATFORM_GL3]->imageFindRasterFormat = imageFindRasterFormat;
	            engine->driver[PLATFORM_GL3]->rasterFromImage    = rasterFromImage;
	            engine->driver[PLATFORM_GL3]->rasterToImage      = rasterToImage;
	        },
	    });
	registerNativeRaster();
}

}
}
