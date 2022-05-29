#pragma once

#include <entt/entity/entity.hpp>
#include <entt/core/hashed_string.hpp>
#include <glm/glm.hpp>

using namespace entt::literals;

namespace timing {
    using Time = float;
    using Delta = Time;
}

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
            Type handle = 0xffffffff;
            // 10 bits (1024) = type, 22 bits (4194304) = id
            const std::uint32_t type () const { return handle >> 22; }
            const std::uint32_t id () const { return handle & 0x003fffff; }
            const bool valid () const { return *this != invalid(); }
            bool operator== (const Handle other) const { return handle == other.handle; }
            bool operator!= (const Handle other) const { return handle != other.handle; }

            static constexpr Handle make (std::uint32_t type, std::uint32_t id) { return Handle{type << 22 | (id & 0x003fffff)}; }
            static constexpr Handle invalid () { return Handle{0xffffffff}; }
        };
    }

    namespace events {
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

            // Post to an entity, no categories filter
            template <typename Message> Message& post (entt::entity target) {
                return *(new (push(Message::ID, entt::to_integral(target), 0x000000, sizeof(Message))) Message{});
            }
            template <typename Message, typename Function> void post (entt::entity target, Function fn) { fn(post<Message>(target)); }
            // Post to an entity, filtered by categories
            template <typename Message> Message& postFiltered (entt::entity target, std::uint16_t categories) {
                return *(new (push(Message::ID, entt::to_integral(target), 0x200000 | categories, sizeof(Message))) Message{});
            }
            template <typename Message, typename Function> void postFiltered (entt::entity target, std::uint16_t categories, Function fn) {
                fn(post<Message>(target, categories));
            }
            // Post to a group, no categories filter
            template <typename Message> Message& post (entt::hashed_string::hash_type target, std::uint16_t categories) {
                return *(new (push(Message::ID, target, 0x400000, sizeof(Message))) Message{});
            }
            template <typename Message, typename Function> void post (entt::hashed_string::hash_type target, std::uint16_t categories, Function fn) {fn(post<Message>(target)); }
            // Post to a group, filtered by categories
            template <typename Message> Message& postFiltered (entt::hashed_string::hash_type target, std::uint16_t categories) {
                return *(new (push(Message::ID, entt::to_integral(target), 0x600000 | categories, sizeof(Message))) Message{});
            }
            template <typename Message, typename Function> void postFiltered (entt::hashed_string::hash_type target, std::uint16_t categories, Function fn) { fn(post<Message>(target, categories)); }

            // Internal! Even though this is public, it should not be used directly.
            virtual std::byte* push (entt::hashed_string::hash_type, std::uint32_t, std::uint32_t, std::uint8_t) = 0;
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

        using EventIterable = Iterable<EventEnvelope>;
    }
}
