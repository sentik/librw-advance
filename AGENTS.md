# AGENTS.md - librw modernization guide

Этот файл задает правила для разработчиков и агентов, которые модернизируют
librw для re3/reVC и похожих проектов. Правила действуют на весь каталог
`vendor/librw`, если более близкий `AGENTS.md` не переопределяет их.

## Цель проекта

librw модернизируется как игровой и 3D-движковый компонент, но должен
сохранять совместимость с RenderWare-подобными форматами, поведением и
backend-архитектурой. Мы улучшаем безопасность, читаемость и поддержку кода,
не превращая миграцию в переписывание ради переписывания.

Главные приоритеты:

1. Сохранить корректность DFF/TXD/BSP-подобных данных, chunk layout,
   endian/layout-ожидания и поведение старых игр.
2. Сохранять работоспособность backend-слоев: D3D8/D3D9, OpenGL/GL3,
   SDL2/SDL3, GLFW, ImGui и tooling.
3. Постепенно заменять C-style код современным C++ с понятным владением,
   строгими типами и стандартными библиотечными решениями.
4. Уменьшать объем ручной памяти, raw arrays, raw callbacks, макросов,
   неявных integer flags и out-parameters.
5. Любое изменение в hot path рендера, streaming, vertex/index layout,
   allocators или state management должно быть небольшим, проверяемым и
   объяснимым.

## Базовый стандарт C++

- Windows/MSVC профиль проекта ориентирован на `C++latest`.
- Практический baseline для нового кода - C++20.
- C++23 можно использовать в новых и изолированных участках, если функция
  поддерживается текущими toolchain-профилями проекта или имеет ясный fallback.
- C++14/17 idioms допустимы при портировании старого кода, но новый API должен
  проектироваться в стиле C++20+.
- Не добавляй нестандартную зависимость, если стандартная библиотека уже дает
  надежное решение.

## Обязательные правила

- Используй готовые решения стандартной библиотеки вместо самодельных
  контейнеров, ручных циклов, ручной памяти и небезопасных callback-схем.
- Для новых enum используй `enum class`, а не plain `enum`.
- Используй `constexpr`, `consteval`, `constinit`, `std::array`,
  `std::span`, `std::string_view`, `std::optional`, `std::expected`,
  `std::variant`, `std::unique_ptr`, `std::shared_ptr`, `std::pmr`,
  ranges/algorithms там, где это повышает безопасность и ясность.
- Raw pointer в новом коде означает "не владею" или interop с C/API.
  Владение выражай через RAII: `std::unique_ptr`, контейнер, handle-wrapper,
  `std::shared_ptr` только при реальном совместном владении.
- Raw C arrays заменяй на `std::array` для фиксированного размера,
  `std::vector`/`std::pmr::vector` для динамики и `std::span` для передачи
  вида на непрерывные данные.
- Размеры буферов должны быть явными. Не полагайся на `sizeof`-макросы в новом
  коде, если можно использовать `.size()`, `std::span`, `std::ssize`.
- Для новых callback API предпочитай `std::function`, шаблонный callable или
  явный интерфейс. Function pointer допустим только для C ABI, legacy tables
  или проверенного hot path.
- Ошибки парсинга, IO, ресурсов и backend-init не скрывай в `bool`.
  Для нового кода предпочитай `std::expected<T, Error>`, `std::error_code`,
  `std::system_error` или хорошо документированную exception-policy.
- Макросы заменяй на `constexpr`, `inline` функции, typed constants,
  attributes и templates, если это не ломает platform/compiler boundary.
- Новый код должен быть warning-clean для MSVC, Clang и GCC настолько, насколько
  это разумно для текущей платформы.

## Когда можно оставить legacy-подход

Оставлять C-style код допустимо временно, если участок:

- является ABI/API boundary с RenderWare-like кодом или внешней C-библиотекой;
- зависит от точного бинарного layout, padding, endian или GPU vertex layout;
- находится в очень горячем render path, где modern abstraction ухудшает
  профиль и это подтверждено измерением;
- относится к vendored-коду ImGui/ImGuizmo/stb/glad и не является нашей
  зоной ответственности.

Даже в таких местах добавляй typed wrappers вокруг опасных операций и
документируй причину исключения рядом с кодом.

## Порядок работы над изменениями

1. Прочитай ближайший код и архитектурный контекст перед правкой.
2. Определи владение памятью и ресурсами: кто создает, кто уничтожает, кто
   только наблюдает.
3. Сохрани поведение тестом, golden-файлом, sample-tool запуском или хотя бы
   проверяемым smoke scenario.
4. Модернизируй маленькими шагами: типы, контейнеры, ownership, errors,
   algorithms/ranges.
5. Для backend-кода проверяй D3D/GL resource lifetime, device/context state,
   thread ownership и cleanup при early return.
6. После изменения запускай доступные сборки/генерацию и фиксируй, что именно
   не удалось проверить.

## Документы с правилами

- [rules/README.md](rules/README.md) - карта rules-документов и быстрый
  чеклист.
- [rules/modern-cpp.md](rules/modern-cpp.md) - язык, типы, ownership,
  constexpr, callbacks и error handling.
- [rules/standard-library.md](rules/standard-library.md) - где и как
  использовать модули стандартной библиотеки C++14/17/20/23.
- [rules/legacy-porting.md](rules/legacy-porting.md) - рецепты портирования
  старого небезопасного кода на современный C++.
- [rules/engine-backends.md](rules/engine-backends.md) - правила для D3D,
  OpenGL, SDL/GLFW, ImGui и движкового runtime.

## Project-local skills

В проекте есть локальные skills в папке `skills/`. Они версионируются вместе с
librw и дополняют правила из `rules/`. Эти skills не нужно загружать все сразу:
выбирай минимальный набор под задачу, чтобы не раздувать контекст.

- `skills/coding-standards/SKILL.md` - используй для задач по стилю,
  именованию, организации файлов, Doxygen-комментариям, readability/KISS/DRY и
  базовым C++20 практикам.
- `skills/cpp-pro/SKILL.md` - используй при написании, review или рефакторинге
  современного C++17/20/23 кода: RAII, type safety, Core Guidelines,
  ownership, error handling, standard library, concurrency и архитектурная
  чистка. Дополнительные файлы этого skill загружай только по необходимости:
  `skills/cpp-pro/modern-cpp.md`, `skills/cpp-pro/concurrency.md`,
  `skills/cpp-pro/templates.md`.
- `skills/cpp-templates-metaprogramming/SKILL.md` - используй для глубоких
  template/metaprogramming задач: specialization, SFINAE, type traits, CRTP,
  variadic templates, constexpr metaprogramming и C++20 concepts.

Если project-local skill конфликтует с `rules/` или с реальным кодом librw,
приоритет такой: корректность движка и форматов, затем `rules/`, затем skill.
Vendored-код ImGui/ImGuizmo/stb/glad не модернизируется только ради
соответствия skill.

Для глобального автоподключения Codex эти skills можно синхронизировать в
`$CODEX_HOME/skills` или `~/.codex/skills`, но исходной версией для проекта
остается папка `skills/` в репозитории.

## Review checklist

Перед завершением изменения проверь:

- Нет нового owning raw pointer, raw `new/delete`, `malloc/free` без причины.
- Нет нового plain `enum`, если это не совместимость с внешним API.
- Нет нового raw C array там, где подходит `std::array`, `std::vector`,
  `std::span` или `std::mdspan`.
- Ошибки не теряются, early return освобождает ресурсы.
- API не заставляет вызывающего угадывать размер буфера или lifetime.
- Стандартная библиотека использована вместо самодельного решения.
- Backend-specific код не протекает в platform-independent слой.
- Поведение старых файлов, pipeline и render state не изменено случайно.
