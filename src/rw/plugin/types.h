#ifndef RW_PLUGIN_TYPES_H
#define RW_PLUGIN_TYPES_H

#include <cstdint>

namespace rw::plugin {

// Strong wrapper над legacy 32-bit plugin id.
// Bit-layout не меняется; меняется только type-safety.
enum class PluginId : std::uint32_t {};

[[nodiscard]] constexpr std::uint32_t toRaw(PluginId id) noexcept
{
    return static_cast<std::uint32_t>(id);
}

[[nodiscard]] constexpr PluginId fromRaw(std::uint32_t raw) noexcept
{
    return static_cast<PluginId>(raw);
}

// Эквивалент макроса MAKEPLUGINID(v, id) из rwbase.h, но typed и
// проверяется на этапе компиляции.
[[nodiscard]] consteval PluginId makePluginId(std::uint32_t vendor,
                                              std::uint32_t id) noexcept
{
    return static_cast<PluginId>(((vendor & 0xFFFFFFu) << 8) | (id & 0xFFu));
}

// Категория плагина. Текущая система кодирует это через size == 0
// и nullable callbacks; новая модель делает категорию явной.
enum class PluginKind : std::uint8_t {
    storageExtension,   // имеет tail-storage аллокацию
    streamOnly,         // 0 байт storage, только read/write/getSize
    rightsOnly,         // 0 байт storage, только rightsCallback
    alwaysCallback,     // 0 байт storage, только alwaysCallback
};

// Политика обработки unknown extension chunk при streamRead.
// Сейчас весь stream работает в режиме optional (unknown → skip).
enum class StreamRequirement : std::uint8_t {
    optional,           // unknown chunk → skip (historic behavior)
    requiredForLoad,    // unknown chunk → std::expected error
    requiredForRender,  // unknown chunk → error + always callbacks не запускаются
};

// Платформа. Численные значения совпадают с rwbase.h::Platform для
// сохранения file-format совместимости.
enum class Platform : std::uint8_t {
    null  = 0,
    gl    = 2,
    ps2   = 4,
    xbox  = 5,
    d3d8  = 8,
    d3d9  = 9,
    wdgl  = 11,
    gl3   = 12,
};

[[nodiscard]] constexpr std::uint8_t toRaw(Platform p) noexcept
{
    return static_cast<std::uint8_t>(p);
}

// Converts a legacy rw::Platform integer value to the typed Platform enum.
[[nodiscard]] constexpr Platform fromPlatformInt(std::uint8_t raw) noexcept
{
    return static_cast<Platform>(raw);
}

// Ошибки регистрации плагина.
enum class RegisterError : std::uint8_t {
    duplicateId,            // плагин с этим id уже зарегистрирован
    frozenRegistry,         // registry уже заморожен (object create начался)
    invalidSize,            // size == 0 для storageExtension
    invalidAlignment,       // alignment не степень двойки или > limit
    missingCallback,        // getSize != null но write == null, и т.п.
    storageRequiredButZero, // storageExtension зарегистрирован с size=0
    invalidPlatform,        // Platform out-of-range для driver registry
    outOfMemory,
};

// Ошибки stream IO в plugin layer.
enum class StreamPluginError : std::uint8_t {
    missingExtensionChunk,  // ID_EXTENSION chunk отсутствует
    truncatedChunk,         // chunk header корректен, payload короче
    unknownRequiredPlugin,  // unknown id с StreamRequirement::required*
    invalidChunkSize,       // chunk length > remaining
    callbackFailed,         // callback вернул std::unexpected
    ioFailure,              // Stream read/write завершился ошибкой
};

// Ошибки создания core object (rw::object::create<T>).
enum class CreateError : std::uint8_t {
    outOfMemory,
    registryNotFrozen,      // create вызван до freezeAll()
    pluginConstructFailed,  // один из plugin construct'ов не смог
};

}   // namespace rw::plugin

#endif  // RW_PLUGIN_TYPES_H
