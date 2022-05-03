#pragma once

#include <entt/core/hashed_string.hpp>
#include <glm/glm.hpp>

using Time = float;
using DeltaTime = Time;

namespace million {

    namespace types {
        enum class Type {
            Vec2,
            Vec3,
            Vec4,
            UInt8,
            UInt16,
            UInt32,
            UInt64,
            Int8,
            Int16,
            Int32,
            Int64,
            Byte,
            Flags8,
            Flags16,
            Flags32,
            Flags64,
            Resource,
            TextureResource,
            MeshResource,
            Entity,
            Float,
            Double,
            Bool,
            Event,
            Ref,
            HashedString,
            RGB,
            RGBA,
            Signal,
        };
    }

    namespace resources {

        struct Handle {
            using Type = std::uint32_t;
            Type handle;
            // 12 bits = type, 20 bits = id
            const std::uint32_t type () const { return handle >> 20; }
            const std::uint32_t id () const { return handle & 0xfffff; }

            const bool valid () const { return handle != invalid().handle; }
            static constexpr Handle invalid () { return Handle{0xffffffff}; }

            static constexpr Handle make (std::uint32_t type, std::uint32_t id) { return Handle{type << 20 | (id & 0xfffff)}; }
        };
    }

    namespace events {
        /// Internal type: not expected to be used directly.
        struct MessageEnvelope { // Message envelope is targetted at a specific entity
            entt::hashed_string::hash_type type;
            entt::entity target;
            std::uint32_t size;
        };

        /// Internal type: not expected to be used directly.
        struct EventEnvelope { // Event envelope is not targetted
            entt::hashed_string::hash_type type;
            std::uint32_t size;
        };

        /// Internal type: not expected to be used directly.
        class Stream
        {
        public:
            virtual ~Stream () {}
            template <typename Event> Event& emit () { return *(new (push(Event::ID, sizeof(Event))) Event{}); }    
            template <typename Event, typename Function> void emit (Function fn) { fn(emit<Event>()); }
            void emit (entt::hashed_string event_id) { push(event_id, 0); }
        protected:
            virtual std::byte* push (entt::hashed_string::hash_type, std::uint32_t) = 0;
        };

        /// Internal type: not expected to be used directly.
        class Publisher
        {
        public:
            virtual ~Publisher () {}
            template <typename Message> Message& post (entt::entity target) { return *(new (push(Message::ID, target, sizeof(Message))) Message{}); }
            template <typename Message, typename Function> void post (entt::entity target, Function fn) { fn(post<Message>(target)); }
        protected:
            virtual std::byte* push (entt::hashed_string::hash_type, entt::entity, std::uint32_t) = 0;
        };

        /// Internal type: not expected to be used directly.
        template <typename EnvelopeType>
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = EnvelopeType;
            using pointer           = const value_type*;
            using reference         = const value_type&;

            Iterator(pointer ptr) : m_ptr(reinterpret_cast<const std::byte*>(ptr)) {}
            Iterator(const std::byte* ptr) : m_ptr(ptr) {}

            reference operator*() const {
                return *reinterpret_cast<pointer>(m_ptr);
            }
            pointer operator->() {
                return reinterpret_cast<pointer>(m_ptr);
            }

            // Prefix increment
            Iterator& operator++() {
                m_ptr += sizeof(value_type) + reinterpret_cast<pointer>(m_ptr)->size;
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
            friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };     

        private:
            const std::byte* m_ptr;
        };
        
        template <typename EnvelopeType>
        struct Iterable {
            Iterable (std::byte* b, std::byte* e) : m_begin_ptr(b), m_end_ptr(e) {}
            Iterator<EnvelopeType> begin() const { return Iterator<EnvelopeType>(m_begin_ptr);}
            Iterator<EnvelopeType> end() const { return Iterator<EnvelopeType>(m_end_ptr);}
            std::size_t size () const { return m_end_ptr - m_begin_ptr; }
        private:
            const std::byte* m_begin_ptr;
            const std::byte* m_end_ptr;
        };

        using MessageIterable = Iterable<MessageEnvelope>;
        using EventIterable = Iterable<EventEnvelope>;
    }
}
