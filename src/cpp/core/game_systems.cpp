
#include "engine.hpp"

// namespace core {
//     class EventStreamPool
//     {
//     public:
//         EventStreamPool (uint32_t size) :
//             m_pools{{size}, {size}},
//             m_current(0)
//         {}
//         ~EventStreamPool () {}

//         million::events::Iterable stream () const
//         {
//             auto& pool = back();
//             return {pool.begin(), pool.end()};
//         }

//         void swap ()
//         {
//             m_current = 1 - m_current;
//             front().reset();
//         }

//         std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size)
//         {
//             std::byte* ptr = front().allocate(sizeof(million::events::Envelope) + payload_size);
//             new (ptr) million::events::Envelope{event_id, source, payload_size};
//             return ptr + sizeof(million::events::Envelope);
//         }

//     private:
//         using PoolType = memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>;
//         PoolType m_pools[2];
//         int m_current;

//         PoolType& front () { return m_pools[m_current]; }
//         const PoolType& back () const { return m_pools[1 - m_current]; }
//     };

//     class EventStream : public million::events::EventStream
//     {
//     public:
//         EventStream (EventStreamPool& pool) : m_pool(pool) {}
//         virtual ~EventStream () {}
//         million::events::Iterable stream () const final { return m_pool.stream(); }

//     private:
//         const EventStreamPool& m_pool;
//     };

//     class OutputStream : public million::events::OutputStream
//     {
//     public:
//         OutputStream (EventStreamPool& pool) : m_pool(pool) {}
//         OutputStream (OutputStream&& other) : m_pool(other.m_pool) {}
//         virtual ~OutputStream () {}

//     private:
//         std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size) final
//         {
//             return m_pool.push(event_id, source, payload_size);
//         }
        
//         EventStreamPool& m_pool;
//     };
// }

// void core::Engine::installGameSystem (entt::hashed_string name, million::systems::CreateFunction create)
// {
//     // auto pool = new core::EventStreamPool(100);
//     // auto output = new core::OutputStream(*pool);
//     // auto system = create(
//     //     *output,
//     //     m_organizers[million::SystemStage::GameLogic],
//     //     m_organizers[million::SystemStage::Update]
//     // );
//     // m_game_systems[name] = {pool, output, system};
// }
