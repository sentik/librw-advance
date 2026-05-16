#include "plugin_list_core.h"
#include "offset.h"        // alignUp

#include "../../rwbase.h"  // rw::Stream, findChunk, ChunkHeaderInfo, ...

#include <algorithm>
#include <cassert>
#include <utility>

namespace rw::plugin {

namespace {

constexpr std::size_t kChunkHeaderBytes = 12;

[[nodiscard]] bool isPow2(std::size_t v) noexcept
{
    return v != 0 && (v & (v - 1)) == 0;
}

}   // namespace

PluginListCore::PluginListCore(std::size_t baseSize) noexcept
    : baseSize_(baseSize)
    , objectSize_(baseSize)
{
}

std::expected<void, RegisterError>
PluginListCore::checkRegisterPrecondition(PluginId id) const noexcept
{
    if(frozen_.load(std::memory_order_acquire))
        return std::unexpected(RegisterError::frozenRegistry);
    for(const auto& d : descriptors_){
        if(d.id == id)
            return std::unexpected(RegisterError::duplicateId);
    }
    return {};
}

std::expected<std::ptrdiff_t, RegisterError>
PluginListCore::addStorageExtension(PluginId id,
                                    std::size_t size,
                                    std::size_t alignment,
                                    PluginLifecycle lifecycle,
                                    PluginStream stream,
                                    PluginDebugInfo debug)
{
    if(auto check = checkRegisterPrecondition(id); !check)
        return std::unexpected(check.error());
    if(size == 0)
        return std::unexpected(RegisterError::storageRequiredButZero);
    if(!isPow2(alignment))
        return std::unexpected(RegisterError::invalidAlignment);
    if(stream.hasGetSize() && !stream.hasWrite())
        return std::unexpected(RegisterError::missingCallback);
    if(stream.hasWrite() && !stream.hasGetSize())
        return std::unexpected(RegisterError::missingCallback);

    const auto offset = static_cast<std::ptrdiff_t>(alignUp(objectSize_, alignment));
    objectSize_ = static_cast<std::size_t>(offset) + size;

    descriptors_.push_back(PluginDescriptor{
        .id        = id,
        .kind      = PluginKind::storageExtension,
        .offset    = offset,
        .size      = size,
        .alignment = alignment,
        .lifecycle = std::move(lifecycle),
        .stream    = std::move(stream),
#ifndef NDEBUG
        .debug     = std::move(debug),
#endif
    });
#ifdef NDEBUG
    (void)debug;
#endif
    ++generation_;
    return offset;
}

std::expected<void, RegisterError>
PluginListCore::addStreamPlugin(PluginId id,
                                PluginStream stream,
                                PluginDebugInfo debug)
{
    if(auto check = checkRegisterPrecondition(id); !check)
        return std::unexpected(check.error());
    if(stream.hasGetSize() != stream.hasWrite())
        return std::unexpected(RegisterError::missingCallback);

    descriptors_.push_back(PluginDescriptor{
        .id        = id,
        .kind      = PluginKind::streamOnly,
        .offset    = 0,
        .size      = 0,
        .alignment = 1,
        .lifecycle = {},
        .stream    = std::move(stream),
#ifndef NDEBUG
        .debug     = std::move(debug),
#endif
    });
#ifdef NDEBUG
    (void)debug;
#endif
    ++generation_;
    return {};
}

std::expected<void, RegisterError>
PluginListCore::addRightsPlugin(PluginId id,
                                PluginStream::Rights rights,
                                PluginDebugInfo debug)
{
    if(auto check = checkRegisterPrecondition(id); !check)
        return std::unexpected(check.error());
    if(!rights)
        return std::unexpected(RegisterError::missingCallback);

    PluginStream stream;
    stream.rights = std::move(rights);
    descriptors_.push_back(PluginDescriptor{
        .id        = id,
        .kind      = PluginKind::rightsOnly,
        .offset    = 0,
        .size      = 0,
        .alignment = 1,
        .lifecycle = {},
        .stream    = std::move(stream),
#ifndef NDEBUG
        .debug     = std::move(debug),
#endif
    });
#ifdef NDEBUG
    (void)debug;
#endif
    ++generation_;
    return {};
}

std::expected<void, RegisterError>
PluginListCore::addAlwaysPlugin(PluginId id,
                                PluginStream::Always always,
                                PluginDebugInfo debug)
{
    if(auto check = checkRegisterPrecondition(id); !check)
        return std::unexpected(check.error());
    if(!always)
        return std::unexpected(RegisterError::missingCallback);

    PluginStream stream;
    stream.always = std::move(always);
    descriptors_.push_back(PluginDescriptor{
        .id        = id,
        .kind      = PluginKind::alwaysCallback,
        .offset    = 0,
        .size      = 0,
        .alignment = 1,
        .lifecycle = {},
        .stream    = std::move(stream),
#ifndef NDEBUG
        .debug     = std::move(debug),
#endif
    });
#ifdef NDEBUG
    (void)debug;
#endif
    ++generation_;
    return {};
}

void PluginListCore::freeze() noexcept
{
    if(frozen_.load(std::memory_order_acquire))
        return;
    byId_.clear();
    byId_.reserve(descriptors_.size());
    for(std::size_t i = 0; i < descriptors_.size(); ++i)
        byId_.emplace_back(descriptors_[i].id, i);
    std::ranges::sort(byId_, {}, &std::pair<PluginId, std::size_t>::first);
    frozen_.store(true, std::memory_order_release);
}

void PluginListCore::construct(void* owner) const noexcept
{
    for(const auto& d : descriptors_){
        if(d.lifecycle.hasConstruct())
            d.lifecycle.construct(owner, d.offset);
    }
}

void PluginListCore::destruct(void* owner) const noexcept
{
    // Обратный порядок: симметричен construct.
    for(auto it = descriptors_.rbegin(); it != descriptors_.rend(); ++it){
        if(it->lifecycle.hasDestruct())
            it->lifecycle.destruct(owner, it->offset);
    }
}

void PluginListCore::copyAll(void* dst, const void* src) const noexcept
{
    for(const auto& d : descriptors_){
        if(d.lifecycle.hasCopy())
            d.lifecycle.copy(dst, src, d.offset);
    }
}

std::expected<void, StreamPluginError>
PluginListCore::streamRead(Stream& stream, void* owner) const noexcept
{
    std::uint32_t length = 0;
    if(!rw::findChunk(&stream, rw::ID_EXTENSION, &length, nullptr))
        return std::unexpected(StreamPluginError::missingExtensionChunk);

    auto remaining = static_cast<std::int64_t>(length);
    while(remaining > 0){
        rw::ChunkHeaderInfo header{};
        if(!rw::readChunkHeaderInfo(&stream, &header))
            return std::unexpected(StreamPluginError::ioFailure);
        remaining -= static_cast<std::int64_t>(kChunkHeaderBytes);
        if(static_cast<std::int64_t>(header.length) > remaining)
            return std::unexpected(StreamPluginError::truncatedChunk);

        const PluginId chunkId = fromRaw(header.type);
        const PluginDescriptor* desc = find(chunkId);

        if(desc != nullptr && desc->stream.hasRead()){
            auto pos0 = stream.tell();
            auto cbResult = desc->stream.read(
                stream,
                static_cast<std::int32_t>(header.length),
                owner,
                desc->offset);
            if(!cbResult)
                return std::unexpected(cbResult.error());
            // Защита: callback должен потребить ровно header.length байт.
            auto pos1 = stream.tell();
            auto consumed = static_cast<std::int64_t>(pos1) -
                            static_cast<std::int64_t>(pos0);
            if(consumed != static_cast<std::int64_t>(header.length))
                return std::unexpected(StreamPluginError::callbackFailed);
        } else if(desc == nullptr &&
                  /* unknown */
                  /* in current behavior — skip */
                  false){
            // Зарезервированный путь для StreamRequirement::required*;
            // если desc отсутствует, текущая модель просто skip-ает
            // (как старый streamRead). Required handling будет добавлен
            // в Phase 9 — пока сохраняем legacy behavior.
            return std::unexpected(StreamPluginError::unknownRequiredPlugin);
        } else {
            // unknown id, или descriptor без read callback → skip payload.
            stream.seek(static_cast<std::int32_t>(header.length));
        }

        remaining -= static_cast<std::int64_t>(header.length);
    }

    // Always callbacks запускаются после успешного чтения всех chunks.
    for(const auto& d : descriptors_){
        if(d.stream.hasAlways())
            d.stream.always(owner, d.offset);
    }
    return {};
}

std::expected<void, StreamPluginError>
PluginListCore::streamWrite(Stream& stream, const void* owner) const noexcept
{
    const auto totalSize = streamGetSize(owner);
    if(!rw::writeChunkHeader(&stream, rw::ID_EXTENSION, totalSize))
        return std::unexpected(StreamPluginError::ioFailure);
    for(const auto& d : descriptors_){
        if(!d.stream.hasGetSize() || !d.stream.hasWrite())
            continue;
        const auto pluginSize = d.stream.getSize(owner, d.offset);
        if(pluginSize <= 0)
            continue;
        if(!rw::writeChunkHeader(&stream,
                                 static_cast<std::int32_t>(toRaw(d.id)),
                                 pluginSize))
            return std::unexpected(StreamPluginError::ioFailure);
        auto cb = d.stream.write(stream, pluginSize, owner, d.offset);
        if(!cb)
            return std::unexpected(cb.error());
    }
    return {};
}

std::int32_t
PluginListCore::streamGetSize(const void* owner) const noexcept
{
    std::int32_t total = 0;
    for(const auto& d : descriptors_){
        if(!d.stream.hasGetSize())
            continue;
        const auto sz = d.stream.getSize(owner, d.offset);
        if(sz > 0)
            total += static_cast<std::int32_t>(kChunkHeaderBytes) + sz;
    }
    return total;
}

void PluginListCore::streamSkip(Stream& stream) const noexcept
{
    std::uint32_t length = 0;
    if(!rw::findChunk(&stream, rw::ID_EXTENSION, &length, nullptr))
        return;
    auto remaining = static_cast<std::int64_t>(length);
    while(remaining > 0){
        rw::ChunkHeaderInfo header{};
        if(!rw::readChunkHeaderInfo(&stream, &header))
            return;
        stream.seek(static_cast<std::int32_t>(header.length));
        remaining -= static_cast<std::int64_t>(kChunkHeaderBytes) +
                     static_cast<std::int64_t>(header.length);
    }
}

void PluginListCore::assertRights(void* owner, PluginId id,
                                  std::uint32_t data) const noexcept
{
    const auto* desc = find(id);
    if(desc != nullptr && desc->stream.hasRights())
        desc->stream.rights(owner, desc->offset, data);
}

std::span<const PluginDescriptor>
PluginListCore::descriptors() const noexcept
{
    return {descriptors_.data(), descriptors_.size()};
}

const PluginDescriptor* PluginListCore::find(PluginId id) const noexcept
{
    if(frozen_.load(std::memory_order_acquire)){
        // O(log N) на горячем streamRead пути.
        auto it = std::ranges::lower_bound(
            byId_, id, {}, &std::pair<PluginId, std::size_t>::first);
        if(it != byId_.end() && it->first == id)
            return &descriptors_[it->second];
        return nullptr;
    }
    // До freeze — линейный поиск (используется только при регистрации).
    for(const auto& d : descriptors_){
        if(d.id == id)
            return &d;
    }
    return nullptr;
}

}   // namespace rw::plugin
