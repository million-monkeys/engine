#pragma once

#include <entt/core/hashed_string.hpp>
#include <glm/glm.hpp>

using Time = float;
using DeltaTime = Time;

namespace monkeys {

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
        struct Handle {};
    }

    namespace events {
        struct Envelope {
            entt::hashed_string::hash_type type;
            entt::entity target;
            std::uint32_t size;
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
            const Iterator begin() const { return Iterator(m_begin_ptr);}
            const Iterator end() const { return Iterator(m_end_ptr);}
        private:
            const std::byte* m_begin_ptr;
            const std::byte* m_end_ptr;
        };
    }
}
