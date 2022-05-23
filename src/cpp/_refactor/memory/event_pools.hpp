#pragma once

// TODO: Replace with better modular design

#include <monkeys.hpp>

namespace memory {
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

        std::byte* push (entt::hashed_string::hash_type message_id, std::uint32_t target, std::uint32_t flags, std::uint8_t payload_size)
        {
            std::byte* ptr = m_pool.allocate(sizeof(MessageEnvelope) + payload_size);
            new (ptr) MessageEnvelope{message_id, target, (std::uint32_t(flags) << 8) | payload_size};
            return ptr + sizeof(MessageEnvelope);
        }

    private:
        PoolType m_pool;

        struct MessageEnvelope { // Message envelope is targetted at a specific entity
            entt::hashed_string::hash_type type;
            std::uint32_t target; // Entity ID or Group ID
            /* Metadata, 32 bits
             * T = 2bits flag, Mask: 0xd0000000, Target type. 00 => target entity, 01 => target group, 10 => target entity set, 11 => target composite
             * F = 1bit flag, Mask: 0x20000000, Filter. 0 => not filtered by category, 1 => filtered by category (only target entities with specified category will receive message)
             * x = reserved
             * C = 16bit bitfield, Mask: 0x00ffff00, Category bitfield, each bit represents one of 14 total possible categories. 0 => Category not filtered by, 1 => category filtered by
             * S = 8bit number, Mask: 0x000000ff, Size of payload in bytes
             */
            std::uint32_t metadata;
        };
    };

    class IterableStream {
    public:
        virtual ~IterableStream() {}
        virtual million::events::EventIterable iter () const = 0;
        virtual void swap () = 0;
    };

    // A double buffered pool.
    using SingleWriterBase = BasePool<heterogeneous::StackPool<alignment::AlignCacheLine>, million::events::EventEnvelope>;
    using MultiWriterBase = BasePool<heterogeneous::AtomicStackPool<alignment::AlignCacheLine>, million::events::EventEnvelope>;

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
        using StreamType = EventStream<StreamPool<MultiWriterBase>>;
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

        std::byte* push (entt::hashed_string::hash_type event_id, std::uint32_t target, std::uint32_t flags, std::uint8_t payload_size)
        {
            return m_pool->push(event_id, target, flags, payload_size);
        }
        
        Pool* m_pool;
    };
}
