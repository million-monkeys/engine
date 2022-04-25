
#include <game.hpp>
#include "engine.hpp"

#include <iterator>
#include <mutex>

std::mutex g_pool_mutex;

using EventPool = core::Engine::EventPool;

thread_local EventPool* g_event_pool = nullptr;

std::byte* core::Engine::allocateEvent (entt::hashed_string::hash_type type, entt::entity target, std::uint8_t payload_size)
{
    if (g_event_pool == nullptr) {
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read
        const std::uint32_t event_pool_size = entt::monostate<"memory/events/pool-size"_hs>();
        g_event_pool = new EventPool(event_pool_size);

        // Only one thread can access event pools list at once
        std::lock_guard<std::mutex> guard(g_pool_mutex);
        m_event_pools.push_back(g_event_pool); // Keep track of this pool so that we can gather the events into a global pool at the end of each frame
    }
    monkeys::events::Envelope* envelope = g_event_pool->emplace<monkeys::events::Envelope>(type, target, payload_size);
    g_event_pool->unaligned_allocate(payload_size);
    return reinterpret_cast<std::byte*>(envelope) + sizeof(monkeys::events::Envelope);
}

// Move events from the thread local pools into the global event pool
void core::Engine::pumpEvents ()
{
    EASY_FUNCTION(profiler::colors::Amber200);
    m_event_pool.reset();
    // Copy thread local events into global pool and reset thread local pools
    for (auto* pool : m_event_pools) {
        m_event_pool.copy<EventPool>(*pool);
        pool->reset();
    }
    refreshEventsIterable();
}

// Make the global event pool visible to consumers
void core::Engine::refreshEventsIterable ()
{
    m_events_iterable = monkeys::events::Iterable{
        m_event_pool.begin(),
        m_event_pool.end()
    };
}

const monkeys::events::Iterable& core::Engine::events ()
{
    return m_events_iterable;
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
