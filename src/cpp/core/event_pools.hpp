#pragma once

#include <game.hpp>

namespace core {
    template <typename PoolT, typename Envelope>
    class BaseEventPool {
    public:
        using PoolType = PoolT;
        static million::events::Iterable<Envelope> iter (const PoolT& pool)
        {
            return {pool.begin(), pool.end()};
        }
    protected:
        static std::byte* push (PoolT& pool, entt::hashed_string::hash_type event_id, entt::entity target, uint32_t payload_size)
        {
            using EnvelopeT = million::events::MessageEnvelope;
            std::byte* ptr = pool.allocate(sizeof(EnvelopeT) + payload_size);
            new (ptr) EnvelopeT{event_id, target, payload_size};
            return ptr + sizeof(EnvelopeT);
        }

        static std::byte* push (PoolT& pool, entt::hashed_string::hash_type event_id, uint32_t payload_size)
        {
            using EnvelopeT = million::events::EventEnvelope;
            std::byte* ptr = pool.allocate(sizeof(EnvelopeT) + payload_size);
            new (ptr) EnvelopeT{event_id, payload_size};
            return ptr + sizeof(EnvelopeT);
        }
    };

    // A simple event pool used for the global event system
    using EventPoolBase = BaseEventPool<memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>, million::events::MessageEnvelope>;
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

        million::events::MessageIterable iter () const
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

    class IterableStream {
    public:
        virtual ~IterableStream() {}
        virtual million::events::EventIterable iter () const = 0;
        virtual void swap () = 0;
    };

    // A double buffered pool.
    using SingleWriterBase = BaseEventPool<memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>, million::events::EventEnvelope>;
    using MultiWriterBase = BaseEventPool<memory::heterogeneous::AtomicStackPool<memory::alignment::AlignCacheLine>, million::events::EventEnvelope>;

    template <typename StreamPoolBase>
    class StreamPool : public StreamPoolBase, public IterableStream
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
        virtual ~StreamPool () {}

        million::events::EventIterable iter () const final
        {
            return Base::iter(back());
        }

        template <typename OtherPool>
        void copyInto (OtherPool& destination) const
        {
            using Pool = typename Base::PoolType;
            destination.template pushAll<Pool>(back());
        }

        void swap () final
        {
            m_current = 1 - m_current;
            front().reset();
        }

        std::byte* push (entt::hashed_string::hash_type event_id, uint32_t payload_size)
        {
            return Base::push(front(), event_id, payload_size);
        }

    private:
        typename Base::PoolType m_pools[2];
        int m_current;

        typename Base::PoolType& front () { return m_pools[m_current]; }
        const typename Base::PoolType& back () const { return m_pools[1 - m_current]; }
    };

    template <typename Pool>
    class EventStream : public million::events::Stream
    {
    public:
        EventStream () : m_pool{nullptr} {}
        EventStream (Pool* pool) : m_pool{pool} {}
        EventStream (EventStream<Pool>&& other) : m_pool(other.m_pool) { other.m_pool = nullptr; }
        virtual ~EventStream () {}

        void operator= (EventStream<Pool>&& other) { m_pool = other.m_pool; other.m_pool = nullptr; }

        bool valid () const { return m_pool != nullptr; }

    private:
        std::byte* push (entt::hashed_string::hash_type event_id, std::uint32_t payload_size)
        {
            return m_pool->push(event_id, payload_size);
        }
        
        Pool* m_pool;
    };

    template <typename Pool>
    class EventPublisher : public million::events::Publisher
    {
    public:
        EventPublisher () : m_pool{nullptr} {}
        EventPublisher (Pool* pool) : m_pool{pool} {}
        EventPublisher(EventPublisher<Pool>&& other) : m_pool(other.m_pool) { other.m_pool = nullptr; }
        virtual ~EventPublisher () {}

        void operator= (EventPublisher<Pool>&& other) { m_pool = other.m_pool; other.m_pool = nullptr; }

        bool valid () const { return m_pool != nullptr; }

    private:
        std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, std::uint32_t payload_size)
        {
            return m_pool->push(event_id, source, payload_size);
        }
        
        Pool* m_pool;
    };
}
