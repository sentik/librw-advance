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
#include "rwxbox.h"
#include "../rw/plugin/driver_platform_registry.h"

namespace rw {
namespace xbox {

void
initMatFX(void)
{
	using namespace rw::plugin;
	(void)DriverPlatformRegistry::instance().registerPlatformLifecycle(
	    PLATFORM_XBOX, fromRaw(ID_MATFX),
	    PluginLifecycle{
	        .construct = [](void*, std::ptrdiff_t) {
	            matFXGlobals.pipelines[PLATFORM_XBOX] = makeMatFXPipeline();
	        },
	        .destruct = [](void*, std::ptrdiff_t) {
	            ((ObjPipeline*)matFXGlobals.pipelines[PLATFORM_XBOX])->destroy();
	            matFXGlobals.pipelines[PLATFORM_XBOX] = nil;
	        },
	    });
}

ObjPipeline*
makeMatFXPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	pipe->instanceCB = defaultInstanceCB;
	pipe->uninstanceCB = defaultUninstanceCB;
	pipe->pluginID = ID_MATFX;
	pipe->pluginData = 0;
	return pipe;
}

}
}
