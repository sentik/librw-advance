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
#include "../rwanim.h"
#include "../rwengine.h"
#include "../rwplugins.h"
#include "rwd3d.h"
#include "rwd3d8.h"
#include "../rw/plugin/driver_platform_registry.h"

namespace rw {
namespace d3d8 {
using namespace d3d;

void
initSkin(void)
{
	using namespace rw::plugin;
	(void)DriverPlatformRegistry::instance().registerPlatformLifecycle(
	    PLATFORM_D3D8, fromRaw(ID_SKIN),
	    PluginLifecycle{
	        .construct = [](void*, std::ptrdiff_t) {
	            skinGlobals.pipelines[PLATFORM_D3D8] = makeSkinPipeline();
	        },
	        .destruct = [](void*, std::ptrdiff_t) {
	            ((ObjPipeline*)skinGlobals.pipelines[PLATFORM_D3D8])->destroy();
	            skinGlobals.pipelines[PLATFORM_D3D8] = nil;
	        },
	    });
}

ObjPipeline*
makeSkinPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	pipe->instanceCB = defaultInstanceCB;
	pipe->uninstanceCB = defaultUninstanceCB;
	pipe->renderCB = defaultRenderCB;
	pipe->pluginID = ID_SKIN;
	pipe->pluginData = 1;
	return pipe;
}

}
}
