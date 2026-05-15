# Porting legacy C/C++ code to modern C++

Этот документ описывает практический путь миграции старого небезопасного кода
librw. Цель - улучшать безопасность и поддержку без случайной смены поведения
RenderWare-like форматов и backend-слоев.

## Общая стратегия

1. Сначала зафиксируй поведение: sample file, tool output, render screenshot,
   test, checksum или manual smoke scenario.
2. Найди ownership всех объектов: кто создает, кто освобождает, кто только
   читает.
3. Замени data shape: raw arrays и `(ptr, count)` на `std::array`,
   `std::vector`, `std::span`, `std::mdspan`.
4. Замени типы: plain `enum` на `enum class`, integer flags на typed flags,
   macros на `constexpr`.
5. Замени lifetime: raw owning pointer на RAII.
6. Замени error shape: `bool + out parameter` на `std::expected`.
7. После каждого шага компилируй и проверяй behavior.

## Raw fixed arrays

До:

```cpp
rw::uint16 indices[] = { 0, 1, 2, 2, 1, 3 };
rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, indices, nelem(indices));
```

После:

```cpp
constexpr std::array<rw::uint16, 6> indices = { 0, 1, 2, 2, 1, 3 };
rw::im3d::RenderIndexedPrimitive(
	rw::PRIMTYPETRILIST,
	indices.data(),
	static_cast<rw::int32>(indices.size()));
```

Следующий шаг - обновить API, чтобы он принимал `std::span<const rw::uint16>`.

```cpp
void renderIndexedPrimitive(PrimitiveType type, std::span<const rw::uint16> indices);
```

## Dynamic buffers

До:

```cpp
auto *data = static_cast<rw::uint8*>(rwNew(size, MEMDUR_FUNCTION));
if(data == nil)
	return false;
read(data, size);
rwFree(data);
```

После:

```cpp
std::vector<std::byte> data(size);
read(std::span<std::byte>{ data });
```

Если буфер временный и создается часто, используй `std::pmr::vector`.

```cpp
std::pmr::monotonic_buffer_resource frameArena;
std::pmr::vector<std::byte> scratch{ &frameArena };
scratch.resize(size);
```

## Owning pointers

До:

```cpp
rw::Texture *texture = rw::Texture::read(name, nil);
if(texture == nil)
	return false;
// ...
rw::Texture::destroy(texture);
```

После:

```cpp
struct TextureDeleter {
	void operator()(rw::Texture* texture) const noexcept
	{
		if(texture != nullptr)
			rw::Texture::destroy(texture);
	}
};

using TexturePtr = std::unique_ptr<rw::Texture, TextureDeleter>;

TexturePtr texture{ rw::Texture::read(name, nullptr) };
if(!texture)
	return std::unexpected(TextureError::notFound);
```

RAII особенно важен в коде D3D/GL/SDL, где early return не должен оставлять
утекший buffer, texture, shader, window или context.

## Plain enum

До:

```cpp
enum Projection {
	PERSPECTIVE,
	PARALLEL
};
```

После:

```cpp
enum class Projection : std::uint8_t {
	perspective,
	parallel
};
```

Если значение пишется в файл или передается внешнему API, явно фиксируй
underlying type и conversion:

```cpp
constexpr std::uint32_t toChunkValue(Projection projection) noexcept
{
	switch(projection){
	case Projection::perspective: return 1u;
	case Projection::parallel: return 2u;
	}
	std::unreachable();
}
```

## Macros

До:

```cpp
#define nelem(A) (sizeof(A) / sizeof A[0])
#define RWTOSTR_(X) #X
```

После для нового кода:

```cpp
template<class T, std::size_t N>
constexpr std::size_t countOf(const T (&)[N]) noexcept
{
	return N;
}

inline constexpr std::string_view rendererName = "gl3";
```

Для diagnostics используй `std::source_location`:

```cpp
void logError(std::string_view message,
	std::source_location location = std::source_location::current());
```

## Out parameters

До:

```cpp
bool readRaster(Stream *stream, Raster **outRaster);
```

После:

```cpp
std::expected<RasterPtr, RasterReadError> readRaster(Stream& stream);
```

Если нужно вернуть несколько значений, создай именованную структуру:

```cpp
struct RasterInfo {
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	RasterFormat format = RasterFormat::unknown;
};

std::expected<RasterInfo, RasterReadError> readRasterInfo(Stream& stream);
```

## Raw callbacks

До:

```cpp
typedef void (*AtomicRenderCallback)(Atomic*);
```

После для нового API:

```cpp
using AtomicRenderCallback = std::function<void(Atomic&)>;
```

Для hot path:

```cpp
template<class Callback>
void forEachVisibleAtomic(std::span<Atomic> atomics, Callback&& callback)
{
	for(Atomic& atomic : atomics){
		if(atomic.visible())
			std::forward<Callback>(callback)(atomic);
	}
}
```

Function pointers можно оставить на legacy Device/Driver boundary, но новый
код вокруг них должен иметь безопасный typed wrapper.

## C strings and paths

До:

```cpp
void setSearchPath(const char *path);
```

После:

```cpp
void setSearchPath(std::filesystem::path path);
```

Для asset names:

```cpp
void setTextureName(std::string_view name);
```

Не смешивай filesystem path и RenderWare texture name в одном `std::string`,
если это разные доменные сущности.

## Manual loops

До:

```cpp
for(int32 i = 0; i < numMaterials; i++){
	if(materials[i]->texture == texture)
		return materials[i];
}
return nil;
```

После:

```cpp
auto it = std::ranges::find(materials, texture, &Material::texture);
return it == materials.end() ? nullptr : *it;
```

Если `materials` пока представлен как raw pointer, сначала сделай view:

```cpp
std::span<Material*> materialView{ materials, static_cast<std::size_t>(numMaterials) };
```

## Binary IO

До:

```cpp
rw::uint32 id;
stream->read(&id, 4);
```

После:

```cpp
std::expected<std::uint32_t, StreamError> id = reader.readU32LE();
if(!id)
	return std::unexpected(id.error());
```

Правила:

- endian всегда должен быть явным;
- размер chunk-а проверяется до чтения;
- overflow при вычислении offsets/sizes проверяется;
- `std::span<std::byte>` используется для raw payload;
- typed structs читаются по полям, а не `reinterpret_cast` всего файла.

## D3D/GL resources

До:

```cpp
GLuint buffer = 0;
glGenBuffers(1, &buffer);
// ...
glDeleteBuffers(1, &buffer);
```

После:

```cpp
class GlBuffer {
public:
	GlBuffer();
	~GlBuffer();

	GlBuffer(const GlBuffer&) = delete;
	GlBuffer& operator=(const GlBuffer&) = delete;
	GlBuffer(GlBuffer&& other) noexcept;
	GlBuffer& operator=(GlBuffer&& other) noexcept;

	[[nodiscard]] GLuint id() const noexcept { return id_; }

private:
	GLuint id_ = 0;
};
```

Для D3D COM resources используй RAII COM holder, если он уже принят в проекте,
или маленький local wrapper с `AddRef/Release` семантикой. Голый `Release()` в
нескольких ветках функции - запах, который надо убирать.

## Состояние миграции

Не обязательно модернизировать файл целиком за один раз. Хороший маленький PR:

- заменяет один тип ресурсов на RAII;
- переводит один API на `std::span`;
- добавляет typed enum для одного набора flags;
- заменяет один parser result на `std::expected`;
- добавляет static_assert для GPU/binary layout;
- оставляет поведение видимо тем же.

## Что не делать

- Не менять layout stream/GPU структур без `static_assert` и проверки.
- Не заменять все raw pointers механически на `shared_ptr`.
- Не превращать все loops в ranges в горячем backend-коде без профиля.
- Не смешивать C++ exceptions и no-exception error style в одном API.
- Не модернизировать vendored ImGui/stb/glad как часть движковой миграции.
- Не делать глобальный рефактор ради стиля без измеримого выигрыша.
