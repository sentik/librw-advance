# Modern C++ rules

Этот документ описывает базовый стиль нового и модернизируемого C++ кода в
librw. Главная идея: типы должны объяснять lifetime, размеры, ошибки и
намерения лучше, чем комментарии.

## Стандарт языка

- Новый код проектируй под C++20.
- В Windows/MSVC профиле допускается `C++latest`, чтобы использовать свежие
  возможности компилятора.
- C++23 функции можно применять в изолированных местах, если они доступны в
  целевых toolchains или имеют понятный fallback.
- Не используй experimental-фичу только потому, что она новая. Она должна
  уменьшать риск, код или сложность.

## Enum и flags

Новые enum всегда пишутся как `enum class`.

Плохо:

```cpp
enum TextureFilter {
	FILTER_NEAREST,
	FILTER_LINEAR
};
```

Хорошо:

```cpp
enum class TextureFilter : std::uint8_t {
	nearest,
	linear
};
```

Для bit flags используй typed enum и helpers:

```cpp
enum class ClearFlags : std::uint32_t {
	color = 1u << 0,
	depth = 1u << 1,
	stencil = 1u << 2
};

constexpr ClearFlags operator|(ClearFlags a, ClearFlags b) noexcept
{
	return static_cast<ClearFlags>(
		static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
```

Plain `enum` допустим только для legacy ABI, C headers или точного совпадения
с внешним API. В таком случае оставь комментарий, почему это boundary.

## Константы и compile-time код

Используй `constexpr` для таблиц, размеров, математических констант,
форматных magic numbers и compile-time conversion.

```cpp
constexpr std::uint32_t rwChunkStruct = 0x0001u;
constexpr std::array<std::uint16_t, 6> quadIndices = { 0, 1, 2, 2, 1, 3 };
```

Используй:

- `constexpr` для обычных compile-time значений и функций;
- `consteval` для функций, которые обязаны выполняться на этапе компиляции;
- `constinit` для global/static объектов, которые должны инициализироваться
  статически;
- `std::is_constant_evaluated` только в низкоуровневых утилитах, где реально
  нужен разный compile-time/runtime путь.

Избегай macro constants:

```cpp
// Плохо
#define RASTER_MAX_MIPS 16

// Хорошо
inline constexpr std::size_t rasterMaxMips = 16;
```

## Владение и lifetime

Raw pointer в новом коде не должен владеть ресурсом.

Используй:

- value type, если объект маленький и копируемый;
- `std::unique_ptr<T>` для единственного владельца;
- `std::shared_ptr<T>` только при реальном совместном владении;
- `std::weak_ptr<T>` для обратных ссылок, чтобы не создать cycle;
- `std::span<T>`/`std::span<const T>` для borrowed buffer;
- reference для обязательного borrowed object;
- `std::optional<std::reference_wrapper<T>>` для необязательной borrowed
  ссылки;
- RAII wrapper для D3D/GL/SDL/ImGui handles.

Плохо:

```cpp
Raster *raster = Raster::create(width, height, depth, flags);
if(!raster)
	return false;
// много кода
Raster::destroy(raster);
```

Хорошо:

```cpp
using RasterPtr = std::unique_ptr<rw::Raster, RasterDeleter>;

RasterPtr raster{ rw::Raster::create(width, height, depth, flags) };
if(!raster)
	return std::unexpected(RenderError::outOfMemory);
```

`std::shared_ptr` не является "более безопасным unique_ptr". Используй его
только когда несколько систем действительно продлевают lifetime объекта:
например, async loader и runtime cache одновременно.

## Массивы, буферы и размеры

Запрещено добавлять новый raw C array для обычных данных, если подходит
стандартный контейнер.

Используй:

- `std::array<T, N>` для фиксированного размера;
- `std::vector<T>` для динамического contiguous buffer;
- `std::pmr::vector<T>` для контролируемой арены;
- `std::span<T>` для параметров функций;
- `std::mdspan<T, ...>` для 2D/3D views на image, mip, vertex grid,
  matrix-like данные, если toolchain поддерживает C++23 `mdspan`;
- `std::string`/`std::string_view` вместо `char*` для текста;
- `std::filesystem::path` для путей в tools и platform layer.

Пример API:

```cpp
void uploadIndices(std::span<const std::uint16_t> indices);
void decodeMip(std::span<const std::byte> src, std::span<std::byte> dst);
```

Не передавай `(ptr, count)` как два независимых параметра в новом API, если
можно передать `std::span`.

## Ошибки и диагностика

Для нового кода выбирай ошибочную модель явно.

Рекомендуется:

- `std::expected<T, Error>` для parsing, IO, resource creation и backend init;
- `std::optional<T>` только когда "нет значения" не является ошибкой;
- `std::error_code`/`std::system_error` для OS/platform failures;
- `assert` для внутренних инвариантов debug-build;
- `std::source_location` для diagnostic helpers;
- `std::stacktrace` в crash/diagnostic tooling, если поддерживается toolchain.

Плохо:

```cpp
bool readTexture(Stream *stream, Texture **outTexture);
```

Хорошо:

```cpp
std::expected<TexturePtr, TextureReadError> readTexture(Stream& stream);
```

Правило: `bool` отвечает на простой вопрос "да/нет". Если есть причина
отказа, нужен typed error.

## Concepts и templates

Используй concepts для template API, который иначе рождает нечитаемые ошибки.

```cpp
template<class T>
concept VertexLike = requires(T v) {
	{ v.position } -> std::same_as<rw::V3d&>;
};

template<VertexLike TVertex>
void transformVertices(std::span<TVertex> vertices, const rw::Matrix& m);
```

Не превращай обычную функцию в template без выгоды. Concepts нужны для
generic utilities, math, serializers, allocators и algorithms, а не для каждой
второй функции.

## Algorithms и ranges

Используй `std::algorithm` и `std::ranges`, когда они делают намерение яснее.

```cpp
std::ranges::sort(meshes, {}, &Mesh::materialId);

auto visibleAtomics = atomics
	| std::views::filter([](const Atomic& a) { return a.visible(); });
```

В hot render loops ручной цикл допустим, если он проще для компилятора,
профилирования или SIMD/cache behavior. Но даже там размеры и views должны
быть typed.

## Callback API

Для новых callback API предпочитай:

- `std::function` для хранимого callback-а с runtime replacement;
- template callable для zero-overhead call site;
- explicit interface, если есть устойчивый набор операций;
- function pointer только для C ABI, legacy tables или измеренного hot path.

Плохо для нового API:

```cpp
typedef void (*RenderCallback)(Atomic*);
```

Хорошо:

```cpp
using RenderCallback = std::function<void(Atomic&)>;
```

В Device/Driver tables старого librw function pointers можно оставлять
временно, но новые обертки вокруг них должны быть typed и RAII-friendly.

## Attributes

Используй стандартные attributes:

- `[[nodiscard]]` для функций, чей результат нельзя игнорировать;
- `[[maybe_unused]]` для platform-specific заглушек;
- `[[fallthrough]]` в switch;
- `[[likely]]`/`[[unlikely]]` осторожно и только в очевидных hot paths;
- `alignas` вместо compiler-specific align macro в новом portable коде.

```cpp
[[nodiscard]] std::expected<Image, ImageError> readImage(Stream& stream);
```

## Names и includes

- Новый код использует `nullptr`, а не `nil`.
- Для стандартных integer types используй `<cstdint>` типы в новом
  platform-independent коде. Legacy `rw::uint32` допустим на API boundary.
- Includes должны быть явными. Не рассчитывай на transitive includes.
- Не добавляй `using namespace std;`.
- В headers минимизируй includes через forward declarations, если это не
  ухудшает читаемость.

## Modules

C++20 modules можно рассматривать для новых, изолированных utility-компонентов
после того, как build-система и все целевые генераторы стабильно это
поддерживают. Не переводи существующие public headers librw в modules одним
большим шагом: это затронет consumers, tools и платформенные сборки.
