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
            std::uint32_t handle;
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
        struct Envelope {
            entt::hashed_string::hash_type type;
            entt::entity target;
            std::uint32_t size;
        };

        /// Internal type: not expected to be used directly.
        class Stream
        {
        public:
            virtual ~Stream () {}
            template <typename Event, typename Function>
            void emit (entt::entity source, Function fn) {
                fn(*(new (push(Event::EventID, source, sizeof(Event))) Event{}));
            }
        protected:
            virtual std::byte* push (entt::hashed_string::hash_type, entt::entity, uint32_t) = 0;
        };

        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Envelope;
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
        
        struct Iterable {
            Iterable (std::byte* b, std::byte* e) : m_begin_ptr(b), m_end_ptr(e) {}
            Iterator begin() const { return Iterator(m_begin_ptr);}
            Iterator end() const { return Iterator(m_end_ptr);}
            std::size_t size () const { return m_end_ptr - m_begin_ptr; }
        private:
            const std::byte* m_begin_ptr;
            const std::byte* m_end_ptr;
        };
    }
}
