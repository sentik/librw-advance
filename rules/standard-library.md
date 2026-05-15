# Standard library usage guide

Правило по умолчанию: если стандартная библиотека уже решает задачу надежно,
используй ее. Самодельные контейнеры, allocators, строки, variant-like union,
manual callbacks, thread primitives и parsing helpers допустимы только после
ясной причины.

## Language support, diagnostics, debugging

Используй:

- `<cstdint>`, `<cstddef>`, `<limits>`, `<type_traits>`, `<utility>` для
  базовых типов и compile-time проверок;
- `std::byte` для raw binary data вместо `uint8_t`, когда данные не являются
  числом;
- `std::endian`, `std::byteswap`, `<bit>` для endian/bit operations;
- `std::source_location` для debug logging, allocation diagnostics,
  stream errors;
- `std::stacktrace` для crash reports и debug tools, если доступно в toolchain;
- `assert`/debug assertions для внутренних инвариантов;
- `std::unreachable` только после доказанной невозможности пути выполнения.

Для librw особенно полезно проверять:

- размер и alignment структур, которые уходят в GPU или binary stream;
- endian assumptions;
- chunk id/version combinations;
- non-null engine/device state перед render calls.

```cpp
static_assert(sizeof(VertexPCT) == 24);
static_assert(std::is_standard_layout_v<VertexPCT>);
```

## Concepts

Используй `<concepts>` для generic-кода, где требуется читаемая диагностика:

- serializers/deserializers;
- math/vector helpers;
- typed resource wrappers;
- allocator-aware containers;
- compile-time format descriptors.

Не используй concepts как украшение. Если обычная overload-функция проще,
оставь обычную функцию.

## Memory management, allocators, smart pointers, pmr

Используй:

- `std::unique_ptr` для единственного владельца;
- `std::shared_ptr` для реального совместного владения;
- `std::weak_ptr` для ссылок назад;
- `std::make_unique` и `std::make_shared`;
- custom deleter для legacy resources;
- `std::pmr::memory_resource` и `std::pmr::*` для arena/frame/stream loading
  allocations;
- `std::span` для borrowed memory;
- `std::allocator_traits` при написании allocator-aware кода.

`std::pmr` хорошо подходит для:

- временной загрузки DFF/TXD;
- построения meshes/material maps во время instancing;
- frame-local temporary buffers;
- tools, где нужно быстро очистить много связанных allocations.

`std::shared_ptr` нельзя использовать как универсальную замену raw pointer.
В render graph, texture cache и async loading он уместен только там, где
lifetime действительно разделяется несколькими владельцами.

## Metaprogramming

Используй:

- `<type_traits>` и `<concepts>` для ограничений типов;
- `constexpr` lookup tables вместо runtime switch там, где это понятно;
- `std::integer_sequence`/fold expressions для compile-time tables;
- `std::variant` вместо unsafe union для tagged data;
- `static_assert` для layout/format contracts.

Не пиши сложную template-магию для обычного gameplay/tooling кода. В librw
metaprogramming полезен в местах форматов, descriptors, serializers и typed
resource wrappers.

## Algorithms, ranges, iterators

Используй:

- `<algorithm>` для sort, find, copy, transform, remove/erase patterns;
- `<numeric>` для accumulate, reduce, transform_reduce;
- `<ranges>` и `std::views` для readable pipelines;
- iterator categories/concepts для generic traversal;
- `std::erase`/`std::erase_if` для контейнеров, где доступно.

Плохо:

```cpp
for(auto i = 0u; i < meshes.size(); ++i)
	if(meshes[i].materialId == materialId)
		return &meshes[i];
```

Хорошо:

```cpp
auto it = std::ranges::find(meshes, materialId, &Mesh::materialId);
return it == meshes.end() ? nullptr : std::addressof(*it);
```

В hot loops рендера ручной цикл допустим, но используй `std::span` и явные
типы размеров.

## General utilities

Используй:

- `std::optional<T>` для отсутствующего значения без ошибки;
- `std::expected<T, E>` для результата с ошибкой;
- `std::variant` для tagged alternatives;
- `std::any` редко, только для plugin/tool metadata без compile-time типа;
- `std::tuple`/`std::pair` для маленьких внутренних групп, но не вместо
  именованной структуры в public API;
- `std::function` для runtime callbacks;
- `std::reference_wrapper` для ссылок в контейнерах или optional reference;
- `std::move`, `std::exchange`, `std::scope_exit`-подобные паттерны, если
  доступен стандартный аналог в toolchain.

`std::expected` особенно полезен для loaders:

```cpp
std::expected<TextureHeader, TextureError> readTextureHeader(Stream& stream);
```

## Input/output, streams, filesystem

Используй:

- `<filesystem>` для путей в tools, tests и platform layer;
- `std::istream`/`std::ostream` для text tools, diagnostics и простого IO;
- `std::spanstream` для чтения/записи в memory buffer, если доступен C++23;
- typed binary reader/writer поверх `std::span<std::byte>` или stream, чтобы
  явно контролировать endian, alignment и chunk sizes;
- `std::error_code` overloads filesystem API, если exceptions нежелательны.

Для runtime binary formats librw не полагайся на formatted iostream extraction.
У RenderWare-like форматов нужны явные endian/layout операции.

`std::system` из `<cstdlib>` не должен использоваться в engine/runtime коде.
В tools он допустим только для явно изолированных developer workflows. Для OS
ошибок используй `std::error_code`, `std::system_error`,
`std::system_category`.

## Containers

Выбирай контейнер по lifetime и layout:

- `std::array<T, N>` - фиксированный размер, compile-time count.
- `std::vector<T>` - основной динамический contiguous контейнер.
- `std::pmr::vector<T>` - vector с контролируемым allocator/resource.
- `std::deque<T>` - стабильные references при росте с двух сторон.
- `std::list`/`std::forward_list` - только если реально нужны stable nodes и
  частые splice/insert/remove в середине. По умолчанию не использовать.
- `std::map`/`std::set` - ordered lookup и стабильные iterators.
- `std::unordered_map`/`std::unordered_set` - hash lookup без порядка.
- `std::flat_map`/`std::flat_set` - C++23 sorted contiguous lookup для малых
  и средних наборов, хорош cache locality.
- `std::span<T>` - borrowed view на contiguous данные.
- `std::mdspan<T, ...>` - view на многомерные данные без владения.
- `std::queue`, `std::stack`, `std::priority_queue` - container adaptors,
  когда нужен именно ограниченный интерфейс.

Для geometry/material/mesh данных чаще всего подходят `std::vector`,
`std::pmr::vector`, `std::span`, `std::array`.

## Concurrency

Используй:

- `std::jthread` вместо `std::thread`, если нужен auto-join и stop token;
- `std::atomic` для lock-free flags/counters с осознанным memory ordering;
- `std::mutex`, `std::scoped_lock`, `std::shared_mutex` для shared state;
- `std::future`/`std::promise` для простого async result;
- `std::counting_semaphore`, `std::binary_semaphore` для producer/consumer;
- `std::latch` для одноразовой синхронизации;
- `std::barrier` для frame/job phases;
- `std::condition_variable` для очередей и ожидания.

Ограничения движка:

- D3D9/OpenGL context обычно принадлежит одному render thread.
- Worker threads могут готовить CPU-side данные, decompress, parse, build
  command/resource descriptions.
- Upload/create GPU resources выполняй на owning render/device thread или
  через явную render command queue.
- ImGui context не трогай из нескольких потоков без отдельной архитектуры.

## Text processing, strings, formatting

Используй:

- `std::string` для owned text;
- `std::string_view` для borrowed text;
- `std::u8string`/`char8_t` для явного UTF-8 API, если это нужно;
- `std::format` для diagnostics и tools, если доступен;
- `std::from_chars`/`std::to_chars` для быстрых locale-free conversions;
- `std::regex` только для tools и не-hot paths.

Для runtime paths лучше `std::filesystem::path`, а для asset identifiers -
typed string/string_view wrappers, чтобы не путать путь, имя текстуры и chunk id.

## Numerics

Используй:

- `<cmath>` и typed overloads для math;
- `<numbers>` для `std::numbers::pi_v<float>` и подобных констант;
- `<numeric>` для reductions;
- `<bit>` для bit_cast, endian, popcount, countl/count r, bit_width;
- `std::lerp`, `std::midpoint` где подходит;
- `std::clamp` вместо ручных clamp-цепочек;
- `std::chrono` для времени и профилирования;
- `std::random` для tools/tests, но не для deterministic game behavior без
  явной seed policy.

Для GPU-facing структур сохраняй explicit float/int sizes и проверяй layout.

## Function objects, references, contracts, debugging, views, generators

Используй:

- lambdas вместо `std::bind`;
- `std::function` для stored callbacks;
- template callable для hot inline callbacks;
- `std::reference_wrapper` для containers of references;
- `std::views` для lazy transforms/filter/take/drop в C++20;
- `std::generator`, если он доступен в toolchain, для tooling/streaming
  итерации, но не для hot render path без измерения;
- standard contracts, когда они станут доступной и включенной частью целевого
  toolchain. До этого используй `assert`, typed precondition helpers и
  `std::expected` для recoverable errors.

Debugging helpers должны быть дешевыми в release или явно отключаемыми.
Нельзя добавлять тяжелую диагностику в frame loop без compile-time/runtime gate.
