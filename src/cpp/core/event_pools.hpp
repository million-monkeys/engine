#pragma once

#include <game.hpp>

namespace core {
    template <typename PoolT, typename Envelope>
    class BasePool {
    public:
        using PoolType = PoolT;
        static million::events::Iterable<Envelope> iter (const PoolT& pool)
        {
            return {pool.begin(), pool.end()};
        }
    protected:
        static std::byte* push (PoolT& pool, entt::hashed_string::hash_type event_id, uint32_t payload_size)
        {
            using EnvelopeT = million::events::EventEnvelope;
            std::byte* ptr = pool.allocate(sizeof(EnvelopeT) + payload_size);
            new (ptr) EnvelopeT{event_id, payload_size};
            return ptr + sizeof(EnvelopeT);
        }
    };

    // A simple event pool used for the global event system
    class MessagePool
    {
    public:
        using PoolType = memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>;

        MessagePool (uint32_t size) :
            m_pool{size}
        {}
        MessagePool (MessagePool&& other)
            : m_pool(std::move(other.m_pool))
        {}
        ~MessagePool () {}

        void reset ()
        {
            m_pool.reset();
        }

        template <typename OtherPool>
        void copyInto (OtherPool& destination) const
        {
            destination.template pushAll<PoolType>(m_pool);
        }

        std::byte* push (entt::hashed_string::hash_type message_id, entt::entity target, uint32_t payload_size)
        {
            using EnvelopeT = million::events::MessageEnvelope;
            std::byte* ptr = m_pool.allocate(sizeof(EnvelopeT) + payload_size);
            new (ptr) EnvelopeT{message_id, target, payload_size};
            return ptr + sizeof(EnvelopeT);
        }

    private:
        PoolType m_pool;
    };

    class IterableStream {
    public:
        virtual ~IterableStream() {}
        virtual million::events::EventIterable iter () const = 0;
        virtual void swap () = 0;
    };

    // A double buffered pool.
    using SingleWriterBase = BasePool<memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>, million::events::EventEnvelope>;
    using MultiWriterBase = BasePool<memory::heterogeneous::AtomicStackPool<memory::alignment::AlignCacheLine>, million::events::EventEnvelope>;

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

    template <typename StreamPoolBase>
    class SingleBufferStreamPool : public StreamPoolBase, public IterableStream
    {
        using Base = StreamPoolBase;
    public:
        SingleBufferStreamPool (uint32_t size) :
            m_pool{size}
        {}
        SingleBufferStreamPool (SingleBufferStreamPool&& other)
            : m_pool{std::move(other.m_pool[0])}
        {}
        virtual ~SingleBufferStreamPool () {}

        million::events::EventIterable iter () const final
        {
            return Base::iter(m_pool);
        }

        void swap () final
        {
            m_pool.reset();
        }

        template <typename OtherPool>
        void copyInto (OtherPool& destination) const
        {
            using Pool = typename Base::PoolType;
            destination.template pushAll<Pool>(m_pool);
        }

        std::byte* push (entt::hashed_string::hash_type event_id, uint32_t payload_size)
        {
            return Base::push(m_pool, event_id, payload_size);
        }

    private:
        typename Base::PoolType m_pool;
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

        friend class RawAccessWrapper;
    };

    // Wrapper to provide access to push raw (type erased) events
    class RawAccessWrapper {
    public:
        using StreamType = core::EventStream<core::StreamPool<core::MultiWriterBase>>;
        RawAccessWrapper(million::events::Stream& stream) : ptr(static_cast<StreamType*>(&stream)) {}
        ~RawAccessWrapper() = default;
        std::byte* push (entt::hashed_string::hash_type event_id, std::uint32_t payload_size)
        {
            return ptr->push(event_id, payload_size);
        }
    private:
        StreamType* ptr;
    };

    template <typename Pool>
    class MessagePublisher : public million::events::Publisher
    {
    public:
        MessagePublisher () : m_pool{nullptr} {}
        MessagePublisher (Pool* pool) : m_pool{pool} {}
        MessagePublisher(MessagePublisher<Pool>&& other) : m_pool(other.m_pool) { other.m_pool = nullptr; }
        virtual ~MessagePublisher () {}

        void operator= (MessagePublisher<Pool>&& other) { m_pool = other.m_pool; other.m_pool = nullptr; }

        bool valid () const { return m_pool != nullptr; }

        std::byte* push (entt::hashed_string::hash_type event_id, entt::entity source, std::uint32_t payload_size)
        {
            return m_pool->push(event_id, source, payload_size);
        }
        
        Pool* m_pool;
    };
}
