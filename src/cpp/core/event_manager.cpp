
#include <game.hpp>
#include "engine.hpp"

#include <iterator>
#include <mutex>

std::mutex g_pool_mutex;

thread_local core::MessagePublisher<core::MessagePool> g_message_publisher;

// Move events from the thread local pools into the global event pool
void core::Engine::pumpEvents ()
{
    EASY_FUNCTION(profiler::colors::Amber200);
    // Swap all event streams internal pools
    for (auto& [name, stream] : m_named_streams) {
        stream.iterable->swap();
    }
    // TODO: Add debug telemetry to track the buffer sizes
}

void core::Engine::pumpMessages ()
{
    m_message_pool.reset();
    // Copy thread local events into global pool and reset thread local pools
    for (auto pool : m_message_pools) {
        pool->copyInto(m_message_pool);
        pool->reset();
    }
}

const million::events::MessageIterable core::Engine::messages () const
{
    return {m_message_pool.begin(), m_message_pool.end()};
}

million::events::Publisher& core::Engine::publisher()
{
    if (! g_message_publisher.valid()) {
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read
        const std::uint32_t message_pool_size = entt::monostate<"memory/events/pool-size"_hs>();

        // Only one thread can access event pools list at once
        std::lock_guard<std::mutex> guard(g_pool_mutex);
        auto message_pool = new MessagePool(message_pool_size);
        m_message_pools.push_back(message_pool); // Keep track of this pool so that we can gather the events into a global pool at the end of each frame
        g_message_publisher = core::MessagePublisher<core::MessagePool>(message_pool);
    }
    return g_message_publisher;
}

million::events::Stream& core::Engine::commandStream()
{
    return m_commands;
}

template <typename StreamBaseType, typename NamedStream>
million::events::Stream& createStreamInternal (entt::hashed_string stream_name, std::uint32_t buffer_size, NamedStream* named_streams)
{
    const std::uint32_t event_stream_size = entt::monostate<"memory/events/stream-size"_hs>();
    buffer_size = buffer_size > 0 ? buffer_size : event_stream_size;
    spdlog::debug("[events] Creating stream '{}' with buffor size of {} bytes", stream_name.data(), buffer_size);
    auto iterable = new core::StreamPool<StreamBaseType>(buffer_size);
    million::events::Stream* streamable = new core::EventStream<core::StreamPool<StreamBaseType>>(iterable);
    named_streams->emplace(stream_name, core::StreamInfo{iterable, streamable});
    return *streamable;
}

million::events::Stream& core::Engine::createStream (entt::hashed_string stream_name, million::StreamWriters writers, std::uint32_t buffer_size)
{
    auto it = m_stream_sizes.find(stream_name.value());
    if (it != m_stream_sizes.end()) {
        buffer_size = it->second;
    }
    switch (writers) {
        case million::StreamWriters::Single:
        {
            // Single Writer stream can be iterated concurrently with writing, but writing must be serialized
            return createStreamInternal<core::SingleWriterBase>(stream_name, buffer_size, &m_named_streams);
        }
        case million::StreamWriters::Multi:
        {
            // Multi Writer stream can be both iterated concurrently with writing and written to concurrently from multiple writer threads, at the cost of updating an atomic integer per write
            return createStreamInternal<core::MultiWriterBase>(stream_name, buffer_size, &m_named_streams);
        }
    };
}

const million::events::EventIterable core::Engine::events (entt::hashed_string stream_name) const
{
    auto it = m_named_streams.find(stream_name);
    if (it != m_named_streams.end()) {
        return it->second.iterable->iter();
    } else {
        spdlog::error("[events] Named stream does not exist: {}", stream_name.data());
        return {nullptr, nullptr};
    }
}

const million::events::EventIterable core::Engine::events (entt::hashed_string::hash_type stream_hash) const
{
    auto it = m_named_streams.find(stream_hash);
    if (it != m_named_streams.end()) {
        return it->second.iterable->iter();
    } else {
        spdlog::error("[events] Event stream does not exist: {}", stream_hash);
        return {nullptr, nullptr};
    }
}
