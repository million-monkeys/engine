
#include "events.hpp"
#include "context.hpp"
#include "config/config.hpp"

#include "core/engine.hpp"

template <typename StreamBaseType, typename NamedStreams, typename... EngineStreams>
million::events::Stream& createStreamHelper (entt::hashed_string stream_name, std::uint32_t buffer_size, NamedStreams* named_streams, EngineStreams*... engine_streams)
{
    const std::uint32_t event_stream_size = entt::monostate<"memory/events/stream-size"_hs>();
    buffer_size = buffer_size > 0 ? buffer_size : event_stream_size;
    SPDLOG_DEBUG("[events] Creating stream '{}' with buffor size of {} bytes", stream_name.data(), buffer_size);
    memory::IterableStream* iterable;
    million::events::Stream* streamable;
    if constexpr (sizeof...(EngineStreams) == 1) {
        auto i = new memory::SingleBufferStreamPool<StreamBaseType>(buffer_size);
        streamable = new memory::EventStream<memory::SingleBufferStreamPool<StreamBaseType>>(i);
        helpers::identity(engine_streams...)->emplace(stream_name, StreamInfo{i, streamable});
        iterable = i;
    } else {
        auto i = new memory::StreamPool<StreamBaseType>(buffer_size);
        streamable = new memory::EventStream<memory::StreamPool<StreamBaseType>>(i);
        iterable = i;
    }
    named_streams->emplace(stream_name, StreamInfo{iterable, streamable});
    return *streamable;
}


million::events::Stream& createStreamInternal (events::Context* context, entt::hashed_string stream_name, million::StreamWriters writers, bool is_engine_stream)
{
    std::uint32_t buffer_size = 0;
    auto& stream_sizes = config::stream_sizes();
    auto it = stream_sizes.find(stream_name.value());
    if (it != stream_sizes.end()) {
        buffer_size = it->second;
    }
    if (is_engine_stream) {
        switch (writers) {
            case million::StreamWriters::Single:
            {
                // Single Writer stream can be iterated concurrently with writing, but writing must be serialized
                return createStreamHelper<memory::SingleWriterBase>(stream_name, buffer_size, &context->m_named_streams, &context->m_engine_streams);
            }
            case million::StreamWriters::Multi:
            {
                // Multi Writer stream can be both iterated concurrently with writing and written to concurrently from multiple writer threads, at the cost of updating an atomic integer per write
                return createStreamHelper<memory::MultiWriterBase>(stream_name, buffer_size, &context->m_named_streams, &context->m_engine_streams);
            }
        };
    } else {
        switch (writers) {
            case million::StreamWriters::Single:
            {
                // Single Writer stream can be iterated concurrently with writing, but writing must be serialized
                return createStreamHelper<memory::SingleWriterBase>(stream_name, buffer_size, &context->m_named_streams);
            }
            case million::StreamWriters::Multi:
            {
                // Multi Writer stream can be both iterated concurrently with writing and written to concurrently from multiple writer threads, at the cost of updating an atomic integer per write
                return createStreamHelper<memory::MultiWriterBase>(stream_name, buffer_size, &context->m_named_streams);
            }
        };
    }
}

void events::createEngineStream (events::Context* context, entt::hashed_string stream_name, million::StreamWriters writers)
{
    EASY_FUNCTION(events::COLOR(3));
    createStreamInternal(context, stream_name, writers, true);
}

million::events::Stream& events::createStream (events::Context* context, entt::hashed_string stream_name, million::StreamWriters writers)
{
    EASY_FUNCTION(events::COLOR(3));
    return createStreamInternal(context, stream_name, writers, false);
}

million::events::Stream& events::commandStream(events::Context* context)
{
    return context->m_commands;
}

void events::pump (events::Context* context)
{
    EASY_BLOCK("events::pump", events::COLOR(2));
    SPDLOG_TRACE("[events] Pumping event pools");
    // Swap all event streams internal pools
    for (auto& [name, stream] : context->m_named_streams) {
        stream.iterable->swap();
    }
    // TODO: Add debug telemetry to track the buffer sizes
}

const million::events::EventIterable events::events (events::Context* context, entt::hashed_string stream_name)
{
    auto it = context->m_named_streams.find(stream_name);
    if (it != context->m_named_streams.end()) {
        return it->second.iterable->iter();
    } else {
        spdlog::error("[events] Named stream does not exist: {}", stream_name.data());
        return {nullptr, nullptr};
    }
}

const million::events::EventIterable events::events (events::Context* context, entt::hashed_string::hash_type stream_hash)
{
    auto it = context->m_named_streams.find(stream_hash);
    if (it != context->m_named_streams.end()) {
        return it->second.iterable->iter();
    } else {
        spdlog::error("[events] Event stream does not exist: {}", stream_hash);
        return {nullptr, nullptr};
    }
}

million::events::Stream* events::engineStream (events::Context* context, entt::hashed_string stream_name)
{
    return context->m_engine_streams[stream_name].streamable;
}
