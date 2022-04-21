
#include <game.hpp>
#include "engine.hpp"

#include <iterator>

using EventPool = core::Engine::EventPool;

thread_local EventPool* g_event_pool = nullptr;

std::byte* core::Engine::requestEvent (entt::hashed_string::hash_type type, entt::entity target, std::uint8_t payload_size)
{
    if (g_event_pool == nullptr) {
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read
        const std::uint32_t event_pool_size = entt::monostate<"memory/events/pool-size"_hs>();
        g_event_pool = new EventPool(event_pool_size);
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

DECLARE_EVENT(MyEvent, "my_event") {
    int foo;
};
DECLARE_EVENT(TestEvent, "test_event") {
};
DECLARE_EVENT(AnotherEvent, "another_event") {
    float value1;
    float value2;
};

void run_system ()
{
    core::Engine engine;
    entt::entity an_entity = entt::null;
    
    // Emit an event
    engine.emit<TestEvent>();
    // Emit an event, receiving reference for setting parameters
    auto& ev = engine.emit<AnotherEvent>();
    ev.value1 = 1.0f;
    ev.value2 = 2.0f;
    // Post event to specific entity 
    auto& myev = engine.emit<MyEvent>(an_entity);
    myev.foo = 123;
}