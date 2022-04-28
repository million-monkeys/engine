#pragma once

#include <game.hpp>

namespace core {
    template <typename PoolT>
    class BaseEventPool {
    public:
        using PoolType = PoolT;
        static million::events::Iterable iter (const PoolT& pool)
        {
            return {pool.begin(), pool.end()};
        }
    protected:
        static std::byte* push (PoolT& pool, entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size)
        {
            std::byte* ptr = pool.allocate(sizeof(million::events::Envelope) + payload_size);
            new (ptr) million::events::Envelope{event_id, source, payload_size};
            return ptr + sizeof(million::events::Envelope);
        }
    };

    // A simple event pool used for the global event system
    using EventPoolBase = BaseEventPool<memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>>;
    class EventPool : public EventPoolBase
    {
        using Base = EventPoolBase;
    public:
        EventPool (uint32_t size) :
            m_pool{size}
        {}
        EventPool (EventPool&& other)
            : m_pool(std::move(other.m_pool))
        {}
        ~EventPool () {}

        million::events::Iterable iter () const
        {
            return Base::iter(m_pool);
        }

        void reset ()
        {
            m_pool.reset();
        }

        template <typename OtherPool>
        void copyInto (OtherPool& destination) const
        {
            destination.template pushAll<Base::PoolType>(m_pool);
        }

        std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size)
        {
            return Base::push(m_pool, event_id, source, payload_size);
        }

    private:
        Base::PoolType m_pool;
    };

    // A double buffered pool.
    using StreamPoolBase = BaseEventPool<memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>>;
    class StreamPool : public StreamPoolBase
    {
        using Base = StreamPoolBase;
    public:
        StreamPool (uint32_t size) :
            m_pools{{size}, {size}},
            m_current(0)
        {}
        StreamPool (StreamPool&& other)
            : m_pools{std::move(other.m_pools[0]), std::move(other.m_pools[1])}
        {}
        ~StreamPool () {}

        million::events::Iterable iter () const
        {
            return Base::iter(back());
        }

        void swap ()
        {
            m_current = 1 - m_current;
            front().reset();
        }

        std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size)
        {
            return Base::push(front(), event_id, source, payload_size);
        }

    private:
        Base::PoolType m_pools[2];
        int m_current;

        Base::PoolType& front () { return m_pools[m_current]; }
        const Base::PoolType& back () const { return m_pools[1 - m_current]; }
    };

    template <typename Pool>
    class EventStream : public million::events::Stream
    {
    public:
        EventStream (Pool& pool) : m_pool{pool} {}
        EventStream (EventStream&& other) : m_pool(other.m_pool) {}
        virtual ~EventStream () {}

    private:
        std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, uint32_t payload_size) final
        {
            return m_pool.push(event_id, source, payload_size);
        }
        
        Pool& m_pool;
    };
}
