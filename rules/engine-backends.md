# Engine and backend rules

Этот документ описывает правила для кода движка, рендера и backend-слоев:
D3D8/D3D9, OpenGL/GL3, SDL2/SDL3, GLFW, ImGui и tools.

## Архитектурные границы

librw имеет несколько уровней:

- platform-independent engine data: Frame, Camera, Geometry, Atomic, Clump,
  Texture, Raster abstractions;
- platform-specific data conversion: Driver;
- render-device code: Device, D3D, OpenGL;
- app/tool shell: SDL/GLFW/windowing, ImGui, sample tools.

Правило: platform-independent слой не должен знать детали D3D/GL/SDL.
Backend-specific типы держи в backend headers/source или в typed opaque
wrappers.

## Resource lifetime

Все backend resources должны иметь понятный owner:

- D3D buffers, textures, shaders, state objects;
- OpenGL buffers, textures, shaders, programs, VAOs, framebuffers;
- SDL/GLFW windows и contexts;
- ImGui context и backend bindings.

Новый backend-код должен использовать RAII. Если API требует ручного destroy,
создай wrapper или `std::unique_ptr` с custom deleter.

```cpp
using SdlWindowPtr = std::unique_ptr<SDL_Window, SdlWindowDeleter>;
```

Early return, exception в tools или failed init не должны оставлять частично
созданные ресурсы.

## Render state

Изменение render state должно быть локальным и восстановимым.

Хорошо:

- state guard для временной смены state;
- явная команда "set pipeline state" перед draw;
- минимизация скрытых globals;
- debug assertions на active device/context.

Плохо:

- менять global state и надеяться, что следующий draw его поправит;
- смешивать texture state, blend state и geometry upload в одной функции без
  ясной причины;
- читать state обратно из GPU API в hot path без необходимости.

## D3D правила

- COM lifetime должен быть RAII-managed.
- Проверяй HRESULT и возвращай typed error.
- Не теряй device lost/reset сценарии.
- D3D9 half-pixel и legacy fixed-function особенности должны быть описаны
  рядом с кодом, который их применяет.
- Vertex declarations, FVF, stride и alignment фиксируй через typed structs и
  `static_assert`.
- Не смешивай D3D8 и D3D9 assumptions в shared коде без явной abstraction.

```cpp
[[nodiscard]] std::expected<D3dBuffer, D3dError> createVertexBuffer(
	Device& device,
	std::span<const VertexPCT> vertices);
```

## OpenGL/GL3 правила

- OpenGL object ids должны жить в movable RAII wrappers.
- Проверяй shader compile/link status и возвращай diagnostic text.
- Не храни GL object id как голый `GLuint` в долгоживущих engine-структурах
  без owner semantics.
- Context ownership должен быть явным. GL calls выполняются на thread-е, где
  context current, если архитектура не говорит обратного.
- VAO/VBO/IBO layout должен быть описан typed descriptors, а не разбросанными
  magic offsets.
- GL debug output включай в debug/developer builds, если доступно.

## SDL, GLFW, windowing

- Event mapping делай через `enum class` и typed conversion.
- Window/context lifetime должен быть RAII-managed.
- Input state не должен зависеть от magic integer constants вне boundary с
  SDL/GLFW.
- Resize, DPI, fullscreen и swap interval должны быть централизованы в shell
  layer, а не размазаны по tools.
- Platform callback-и преобразуй в engine events как можно ближе к boundary.

## ImGui

- Vendored ImGui/ImGuizmo код не модернизируется как часть librw cleanup.
- Наш backend/wrapper вокруг ImGui должен быть RAII-safe.
- ImGui context должен иметь явного owner и shutdown path.
- Не держи raw pointers на ImGui data вне кадра, если ImGui не гарантирует
  lifetime.
- Tools UI может использовать современный C++ активнее, чем runtime core,
  потому что он меньше связан с binary compatibility.

## Geometry, vertex/index data

- CPU-side geometry хранится в стандартных контейнерах или spans.
- GPU upload принимает `std::span<const Vertex>` и
  `std::span<const std::uint16_t/std::uint32_t>`.
- Vertex structs должны быть standard-layout, с явным alignment и
  `static_assert(sizeof(...))`.
- Не используй `reinterpret_cast` для чтения файла прямо в GPU struct без
  проверки endian, version и padding.
- Для индексов выбирай `std::uint16_t` или `std::uint32_t` явно.
- Для multi-dimensional image/mip data используй `std::mdspan` или маленький
  typed view, если `mdspan` недоступен.

## Streaming and file formats

- Chunk id, version, platform id и native raster format должны быть typed.
- При чтении всегда проверяй chunk size до allocation.
- Любое вычисление `offset + size`, `width * height * bpp`, mip chain size
  проверяй на overflow.
- В новом parser-коде возвращай `std::expected`.
- Не полагайся на host endian.
- Старые форматы могут иметь странности. Совместимость важнее красивой модели,
  но странность должна быть локализована и названа.

## Concurrency in engine code

- Render device/context принадлежит одному thread-у, пока не доказано обратное.
- Worker threads могут делать CPU preparation: decode, parse, build mesh
  descriptions, compress/decompress textures.
- GPU resource creation/upload идет через render thread или command queue.
- Shared caches защищай mutex/shared_mutex или lock-free структурой с ясной
  memory ordering.
- `std::jthread` и stop tokens предпочтительнее manual thread lifetime.
- `std::barrier`/`std::latch` подходят для frame/job phases.

## Performance policy

Современный C++ не должен ухудшать predictable performance.

Используй:

- `std::span` для zero-copy views;
- `std::pmr` для массовых временных allocations;
- contiguous containers для geometry/material/mesh;
- typed small structs вместо untyped maps там, где данные идут в hot path;
- profiling перед заменой hot loop на более абстрактный код.

Не используй:

- `std::function` в inner draw loop без измерения;
- shared ownership для каждого resource "на всякий случай";
- linked lists без реальной причины;
- exceptions в тесном runtime path, если проект выбрал expected/error-code
  модель.

## Backend change checklist

Перед merge backend-изменения проверь:

- resource lifetime покрывает failed init и shutdown;
- device/context ownership не нарушен;
- render state после функции предсказуем;
- HRESULT/GL errors/SDL errors не игнорируются;
- vertex/index layout проверен `static_assert`;
- platform-independent API не получил D3D/GL типы;
- tools и samples продолжают запускаться в соответствующем профиле;
- debug symbols и diagnostics помогают понять сбой, а не скрывают его.
