
#include <game.hpp>
#include "engine.hpp"

#include <iterator>
#include <mutex>

std::mutex g_pool_mutex;

thread_local core::EventPublisher<core::EventPool> g_event_publisher;

core::EventPool* g_scripts_event_pool;

// Move events from the thread local pools into the global event pool
void core::Engine::pumpEvents ()
{
    EASY_FUNCTION(profiler::colors::Amber200);
    m_event_pool.reset();
    // Copy thread local events into global pool and reset thread local pools
    for (auto& pool : m_event_pools) {
        pool.copyInto(m_event_pool);
        pool.reset();
    }
    // Swap all event streams internal pools
    for (auto& [name, stream] : m_named_streams) {
        stream.iterable->swap();
    }
}

void core::Engine::pumpScriptEvents ()
{
    // Copy script event pool into global pool
    g_scripts_event_pool->copyInto(m_event_pool);
    // Clear the script event pool
    g_scripts_event_pool->reset();
}

const million::events::MessageIterable core::Engine::messages () const
{
    return EventPoolBase::iter(m_event_pool);
}

million::events::Publisher& core::Engine::publisher()
{
    if (! g_event_publisher.valid()) {
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read
        const std::uint32_t event_pool_size = entt::monostate<"memory/events/pool-size"_hs>();

        // Only one thread can access event pools list at once
        std::lock_guard<std::mutex> guard(g_pool_mutex);
        m_event_pools.emplace_back(event_pool_size); // Keep track of this pool so that we can gather the events into a global pool at the end of each frame
        g_event_publisher = core::EventPublisher<core::EventPool>(&m_event_pools.back());
    }
    return g_event_publisher;
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
    auto it = m_stream_sizes.find(stream_name);
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
