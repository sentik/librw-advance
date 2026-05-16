#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rwbase.h"
#include "rw/plugin/object_registry.h"
#include "rwerror.h"
#include "rwplg.h"
#include "rwpipeline.h"
#include "rwscene.h"
#include "rwgeometry.h"
#include "rwtexture.h"
#include "rwengine.h"
#include "rwuserdata.h"

#define PLUGIN_ID ID_USERDATA

namespace rw {

UserDataGlobals userDataGlobals;

#define udMalloc(sz) rwMalloc(sz, MEMDUR_EVENT | ID_USERDATA)

void
UserDataArray::setString(int32 n, const char *s)
{
	int32 len;
	char **sp = &((char**)this->data)[n];
	rwFree(*sp);
	len = (int32)strlen(s)+1;
	*sp = (char*)udMalloc(len);
	if(*sp)
		strncpy(*sp, s, len);
}

static void*
createUserData(void *object, std::ptrdiff_t offset, int32)
{
	UserDataExtension *ext = (UserDataExtension*)((uint8*)object + offset);
	ext->numArrays = 0;
	ext->arrays = nil;
	return object;
}

static void*
destroyUserData(void *object, std::ptrdiff_t offset, int32)
{
	int32 i, j;
	char **strar;
	UserDataArray *a;
	UserDataExtension *ext = (UserDataExtension*)((uint8*)object + offset);
	a = ext->arrays;
	for(i = 0; i < ext->numArrays; i++){
		rwFree(a->name);
		switch(a->datatype){
		case USERDATASTRING:
			strar = (char**)a->data;
			for(j = 0; j < a->numElements; j++)
				rwFree(strar[j]);
			/* fallthrough */
		case USERDATAINT:
		case USERDATAFLOAT:
			rwFree(a->data);
			break;
		}
		a++;
	}
	rwFree(ext->arrays);
	ext->numArrays = 0;
	ext->arrays = nil;
	return object;
}

static void*
copyUserData(void *dst, void *src, std::ptrdiff_t offset, int32)
{
	int32 i, j;
	char **srcstrar, **dststrar;
	UserDataArray *srca, *dsta;
	UserDataExtension *srcext = (UserDataExtension*)((uint8*)src + offset);
	UserDataExtension *dstext = (UserDataExtension*)((uint8*)dst + offset);
	dstext->numArrays = srcext->numArrays;
	dstext->arrays = (UserDataArray*)udMalloc(dstext->numArrays*sizeof(UserDataArray));
	srca = srcext->arrays;
	dsta = srcext->arrays;
	for(i = 0; i < srcext->numArrays; i++){
		int32 len = (int32)strlen(srca->name)+1;
		dsta->name = (char*)udMalloc(len);
		strncpy(dsta->name, srca->name, len);
		dsta->datatype = srca->datatype;
		dsta->numElements = srca->numElements;
		switch(srca->datatype){
		case USERDATAINT:
			dsta->data = (int32*)udMalloc(sizeof(int32)*dsta->numElements);
			memcpy(dsta->data, srca->data, sizeof(int32)*dsta->numElements);
			break;
		case USERDATAFLOAT:
			dsta->data = (float32*)udMalloc(sizeof(float32)*dsta->numElements);
			memcpy(dsta->data, srca->data, sizeof(float32)*dsta->numElements);
			break;
		case USERDATASTRING:
			dststrar = (char**)udMalloc(sizeof(char*)*dsta->numElements);
			dsta->data = dststrar;
			srcstrar = (char**)srca->data;
			for(j = 0; j < dsta->numElements; j++){
				len = (int32)strlen(srcstrar[j])+1;
				dststrar[j] = (char*)udMalloc(len);
				strncpy(dststrar[j], srcstrar[j], len);
			}
			break;
		}
		srca++;
		dsta++;
	}
	return dst;
}

static Stream*
readUserData(Stream *stream, int32, void *object, std::ptrdiff_t offset, int32)
{
	int32 i, j;
	char **strar;
	UserDataArray *a;
	UserDataExtension *ext = (UserDataExtension*)((uint8*)object + offset);
	ext->numArrays = stream->readI32();
	ext->arrays = (UserDataArray*)udMalloc(ext->numArrays*sizeof(UserDataArray));
	a = ext->arrays;
	for(i = 0; i < ext->numArrays; i++){
		int32 len = stream->readI32();
		a->name = (char*)udMalloc(len);
		stream->read8(a->name, len);
		a->datatype = stream->readU32();
		a->numElements = stream->readI32();
		switch(a->datatype){
		case USERDATAINT:
			a->data = (int32*)udMalloc(sizeof(int32)*a->numElements);
			stream->read32(a->data, sizeof(int32)*a->numElements);
			break;
		case USERDATAFLOAT:
			a->data = (float32*)udMalloc(sizeof(float32)*a->numElements);
			stream->read32(a->data, sizeof(float32)*a->numElements);
			break;
		case USERDATASTRING:
			strar = (char**)udMalloc(sizeof(char*)*a->numElements);
			a->data = strar;
			for(j = 0; j < a->numElements; j++){
				len = stream->readI32();
				strar[j] = (char*)udMalloc(len);
				stream->read8(strar[j], len);
			}
			break;
		}
		a++;
	}
	return stream;
}

static Stream*
writeUserData(Stream *stream, int32, void *object, std::ptrdiff_t offset, int32)
{
	int32 len;
	int32 i, j;
	char **strar;
	UserDataArray *a;
	UserDataExtension *ext = (UserDataExtension*)((uint8*)object + offset);
	stream->writeI32(ext->numArrays);
	a = ext->arrays;
	for(i = 0; i < ext->numArrays; i++){
		len = (int32)strlen(a->name)+1;
		stream->writeI32(len);
		stream->write8(a->name, len);
		stream->writeU32(a->datatype);
		stream->writeI32(a->numElements);
		switch(a->datatype){
		case USERDATAINT:
			stream->write32(a->data, sizeof(int32)*a->numElements);
			break;
		case USERDATAFLOAT:
			stream->write32(a->data, sizeof(float32)*a->numElements);
			break;
		case USERDATASTRING:
			strar = (char**)a->data;
			for(j = 0; j < a->numElements; j++){
				len = (int32)strlen(strar[j])+1;
				stream->writeI32(len);
				stream->write8(strar[j], len);
			}
			break;
		}
		a++;
	}
	return stream;
}

static int32
getSizeUserData(void *object, std::ptrdiff_t offset, int32)
{
	int32 len;
	int32 i, j;
	char **strar;
	int32 size;
	UserDataArray *a;
	UserDataExtension *ext = (UserDataExtension*)((uint8*)object + offset);
	if(ext->numArrays == 0)
		return 0;
	size = 4;	// numArrays
	a = ext->arrays;
	for(i = 0; i < ext->numArrays; i++){
		len = (int32)strlen(a->name)+1;
		size += 4 + len + 4 + 4;	// name len, name, type, numElements
		switch(a->datatype){
		case USERDATAINT:
			size += sizeof(int32)*a->numElements;
			break;
		case USERDATAFLOAT:
			size += sizeof(float32)*a->numElements;
			break;
		case USERDATASTRING:
			strar = (char**)a->data;
			for(j = 0; j < a->numElements; j++){
				len = (int32)strlen(strar[j])+1;
				size += 4 + len;	// len and string
			}
			break;
		}
		a++;
	}
	return size;
}

int32
UserDataExtension::add(const char *name, int32 datatype, int32 numElements)
{
	int32 i;
	int32 len;
	int32 typesz;
	UserDataArray *a;
	// try to find empty slot
	for(i = 0; i < this->numArrays; i++)
		if(this->arrays[i].datatype == USERDATANA)
			goto alloc;
	// have to realloc
	a = (UserDataArray*)udMalloc((this->numArrays+1)*sizeof(UserDataArray));
	if(a == nil)
		return -1;
	memcpy(a, this->arrays, this->numArrays*sizeof(UserDataArray));
	rwFree(this->arrays);
	this->arrays = a;
	i = this->numArrays++;
alloc:
	a = &this->arrays[i];
	len = (int32)strlen(name)+1;
	a->name = (char*)udMalloc(len+1);
	assert(a->name);
	strncpy(a->name, name, len);
	a->datatype = datatype;
	a->numElements = numElements;
	typesz = datatype == USERDATAINT ? sizeof(int32) :
		datatype == USERDATAFLOAT ? sizeof(float32) :
		datatype == USERDATASTRING ? sizeof(char*) : 0;
	a->data = udMalloc(typesz*numElements);
	assert(a->data);
	memset(a->data, 0, typesz*numElements);
	return i;
}

void
UserDataExtension::remove(int32 n)
{
	int32 i;
	UserDataArray *a = &this->arrays[n];
	if(a->name){
		rwFree(a->name);
		a->name = nil;
	}
	if(a->datatype == USERDATASTRING)
		for(i = 0; i < a->numElements; i++)
			rwFree(((char**)a->data)[i]);
	if(a->data){
		rwFree(a->data);
		a->data = nil;
	}
	a->datatype = USERDATANA;
	a->numElements = 0;
}

int32
UserDataExtension::findIndex(const char *name) {
	for(int32 i = 0; i < this->numArrays; i++)
		if(strcmp(this->arrays[i].name, name) == 0)
			return i;
	return -1;
}

#define ACCESSOR(TYPE, NAME) \
int32 \
UserDataArray::NAME##Add(TYPE *t, const char *name, int32 datatype, int32 numElements) \
{ \
	return plugin::extensionPtr(t, userDataGlobals.NAME##Offset)->add(name, datatype, numElements); \
} \
void \
UserDataArray::NAME##Remove(TYPE *t, int32 n) \
{ \
	plugin::extensionPtr(t, userDataGlobals.NAME##Offset)->remove(n); \
} \
int32 \
UserDataArray::NAME##GetCount(TYPE *t) \
{ \
	return plugin::extensionPtr(t, userDataGlobals.NAME##Offset)->getCount(); \
} \
UserDataArray* \
UserDataArray::NAME##Get(TYPE *t, int32 n) \
{ \
	return plugin::extensionPtr(t, userDataGlobals.NAME##Offset)->get(n); \
} \
int32 \
UserDataArray::NAME##FindIndex(TYPE *t, const char *name) \
{ \
	return plugin::extensionPtr(t, userDataGlobals.NAME##Offset)->findIndex(name); \
}

ACCESSOR(Geometry, geometry)
ACCESSOR(Frame, frame)
ACCESSOR(Camera, camera)
ACCESSOR(Light, light)
ACCESSOR(Material, material)
ACCESSOR(Texture, texture)

UserDataExtension *UserDataExtension::get(Geometry *geo) { return plugin::extensionPtr(geo, userDataGlobals.geometryOffset); }
UserDataExtension *UserDataExtension::get(Frame *frame) { return plugin::extensionPtr(frame, userDataGlobals.frameOffset); }
UserDataExtension *UserDataExtension::get(Camera *cam) { return plugin::extensionPtr(cam, userDataGlobals.cameraOffset); }
UserDataExtension *UserDataExtension::get(Light *light) { return plugin::extensionPtr(light, userDataGlobals.lightOffset); }
UserDataExtension *UserDataExtension::get(Material *mat) { return plugin::extensionPtr(mat, userDataGlobals.materialOffset); }
UserDataExtension *UserDataExtension::get(Texture *tex) { return plugin::extensionPtr(tex, userDataGlobals.textureOffset); }

void
registerUserDataPlugin(void)
{
	// TODO: World Sector

	// Geometry — fully migrated to the typed plugin layer.
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Geometry>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-geometry");
		if(result)
			userDataGlobals.geometryOffset = *result;
	}

	// Frame — fully migrated to the typed plugin layer (hanim + userdata both moved).
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Frame>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-frame");
		if(result)
			userDataGlobals.frameOffset = *result;
	}

	// Camera — fully migrated to the typed plugin layer.
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Camera>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-camera");
		if(result)
			userDataGlobals.cameraOffset = *result;
	}

	// Light — fully migrated to the typed plugin layer.
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Light>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-light");
		if(result)
			userDataGlobals.lightOffset = *result;
	}

	// Material — fully migrated to the typed plugin layer.
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Material>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-material");
		if(result)
			userDataGlobals.materialOffset = *result;
	}

	// Texture is fully migrated to the typed plugin layer.
	{
		using namespace rw::plugin;
		auto& reg = ObjectRegistry<Texture>::instance();
		PluginLifecycle lc{
			.construct = [](void* o, std::ptrdiff_t off) {
				createUserData(o, off, 0);
			},
			.destruct = [](void* o, std::ptrdiff_t off) {
				destroyUserData(o, off, 0);
			},
			.copy = [](void* d, const void* s, std::ptrdiff_t off) {
				copyUserData(d, const_cast<void*>(s), off, 0);
			},
		};
		PluginStream st{
			.read = [](Stream& s, std::int32_t len, void* o, std::ptrdiff_t off)
			        -> std::expected<void, StreamPluginError> {
				auto* r = readUserData(&s, len, o, off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.write = [](Stream& s, std::int32_t len, const void* o, std::ptrdiff_t off)
			         -> std::expected<void, StreamPluginError> {
				auto* r = writeUserData(&s, len, const_cast<void*>(o), off, 0);
				return r ? std::expected<void, StreamPluginError>{} :
				           std::unexpected(StreamPluginError::ioFailure);
			},
			.getSize = [](const void* o, std::ptrdiff_t off) -> std::int32_t {
				return getSizeUserData(const_cast<void*>(o), off, 0);
			},
		};
		auto result = reg.registerExtension<UserDataExtension>(
		    fromRaw(ID_USERDATA), std::move(lc), std::move(st), "userdata-texture");
		if(result)
			userDataGlobals.textureOffset = *result;
	}
}

}
