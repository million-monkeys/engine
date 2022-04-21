#pragma once

#include "alignment.hpp"
#include "out_of_space_policies.hpp"

namespace heterogeneous {
    template <typename PoolAlign = alignment::NoAlign, typename ItemAlign = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class StackPool {
    private:
        template <typename T>
        T* alloc () {
            return ItemAlign::template align<T>(unaligned_allocate(ItemAlign::adjust_size(sizeof(T))));
        }
    public:
        using PoolAlignType = PoolAlign;
        using ItemAlignType = ItemAlign;
        using OutOfSpacePolicyType = OutOfSpacePolicy;

        StackPool (std::uint32_t size) :
            size(PoolAlign::adjust_size(size)),
            buffer(new std::byte[size]),
            base(PoolAlign::template align<std::byte>(buffer)),
            next(0),
            items(0) {

        }
        ~StackPool() {
            delete [] buffer;
        }

        // Allocate, but don't construct
        std::byte* unaligned_allocate (std::uint32_t bytes) {
            if (next + bytes <= size) {
                ++items;
                std::byte* ptr = base + next;
                next = next + bytes;
                return ptr;
            } else {
                return OutOfSpacePolicy::template apply<std::byte>("heterogeneous::StackPool");
            }
        }

        std::byte* allocate (std::uint32_t bytes) {
            return ItemAlign::template align<std::byte>(unaligned_allocate(ItemAlign::adjust_size(bytes)));
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
        template <typename SP>
        void copy (StackPool<typename SP::PoolAlignType, typename SP::ItemAlignType, typename SP::OutOfSpacePolicyType>& other) {
            if (next + other.next <= size) {
                std::memcpy(reinterpret_cast<void*>(base + next), reinterpret_cast<const void*>(other.base), other.next);
                next += other.next;
            } else {
                OutOfSpacePolicy::template apply<void>("heterogeneous::StackPool");
            }
        }

        std::byte* begin () const {
            return base;
        }
        
        std::byte* end () const {
            return base + next;
        }

    private:
        const std::uint32_t size;
        std::byte* const buffer;
        std::byte* const base;
        std::uint32_t next;
        std::uint32_t items;
    };
}
