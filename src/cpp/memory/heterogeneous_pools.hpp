#pragma once

#include "alignment.hpp"

namespace heterogeneous {
    template <typename Align = alignment::NoAlign>
    class StackPool {
    private:
        template <typename T>
        T* alloc () {
            return Align::align<T>(nallocate(sizeof(T)));
        }
    public:
        using Type = T;
        using AlignType = Align;

        StackPool (std::uint32_t size) :
            memory(new std::byte[size]),
            next(0),
            size(size),
            items(0) {

        }
        ~StackPool() {
            delete [] memory;
        }

        // Allocate, but don't construct
        std::byte* allocate (std::uint32_t bytes) {
            if (next + bytes < size) {
                ++items;
                std::byte* ptr = memory + next;
                next = next + Align::adjust_size(bytes);
                return object;
            } else {
                throw std::runtime_error("StackPool allocated more items than reserved space");
            }
        }
            
        // Allocate and construct
        template <typename T, typename... Args>
        T* emplace (Args&&... args) {
            return new(alloc<T>()) T{args...};
        }

        template <typename T>
        void push_back (const T& item) {
            std::memcpy(reinterpret_cast<void*>(alloc<T>()), reinterpret_cast<const void*>(item), sizeof(T));
        }

        void reset () {
            next = 0;
            items = 0;
        }

        std::uint32_t remaining () const {
            return size - next;
        }

        std::uint32_t count () const {
            return items;
        }

        std::uint32_t capacity () const {
            return size;
        }

        // Copy items from other into StackPool
        template <typename PT>
        void copy (StackPool<typename PT::AlignType>& other) {
            std::memcpy(reinterpret_cast<void*>(memory), reinterpret_cast<const void*>(other.pool), next);
        }

    private:
        std::byte* const memory;
        std::uint32_t next;
        const std::uint32_t size;
        std::uint32_t items;
    };
}
