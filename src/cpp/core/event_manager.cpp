
#include <game.hpp>
#include "engine.hpp"

#include <iterator>
#include <mutex>

std::mutex g_pool_mutex;

thread_local core::EventPool* g_event_pool = nullptr;

std::byte* core::Engine::allocateEvent (entt::hashed_string::hash_type type, entt::entity target, std::uint8_t payload_size)
{
    if (g_event_pool == nullptr) {
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read
        const std::uint32_t event_pool_size = entt::monostate<"memory/events/pool-size"_hs>();

        // Only one thread can access event pools list at once
        std::lock_guard<std::mutex> guard(g_pool_mutex);
        m_event_pools.emplace_back(event_pool_size); // Keep track of this pool so that we can gather the events into a global pool at the end of each frame
        g_event_pool = &m_event_pools.back();
    }
    return g_event_pool->push(type, target, payload_size);
}

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
        stream.pool->swap();
    }
    // Copy script event pool into global pool
    m_scripts_event_pool.copyInto(m_event_pool);
    // Clear the script event pool
    m_scripts_event_pool.reset();
}

const million::events::Iterable core::Engine::events () const
{
    return EventPoolBase::iter(m_event_pool);
}

million::events::Stream& core::Engine::createStream (entt::hashed_string stream_name)
{
    const std::uint32_t event_stream_size = entt::monostate<"memory/events/stream-size"_hs>();
    auto pool = new core::StreamPool(event_stream_size);
    auto& inserted = *m_named_streams.emplace(stream_name, StreamInfo{pool, {*pool}}).first;
    return inserted.second.stream;
}

const million::events::Iterable core::Engine::events (entt::hashed_string stream_name) const
{
    auto it = m_named_streams.find(stream_name);
    if (it != m_named_streams.end()) {
        return it->second.pool->iter();
    } else {
        spdlog::error("[events] Named stream does not exist: {}", stream_name.data());
        return {nullptr, nullptr};
    }
}

// namespace events {

//     using ID = entt::hashed_string;

//     namespace internal {
//         void declare (ID name, entt::id_type type_id, std::uint8_t size)
//         {
// #ifdef DEBUG_BUILD
//             if (size > 62) {
//                 spdlog::error("Event {} is {} bytes in size, must not exceed 62 bytes", name.data(), size);
//                 throw std::logic_error("Declared event struct is too large");
//             }
// #endif
//         }

//     }
    
//     template <typename T, typename... Rest>
//     void declare ()
//     {
//         internal::declare(EventNameRegistry<T>::EventID, entt::type_index<T>::value(), sizeof(T));
//         if constexpr (sizeof...(Rest) > 0) {
//             declare<Rest...>();
//         }
//     }
// }

