#pragma once

#include "alignment.hpp"
#include "out_of_space_policies.hpp"

#include <atomic>

#include <spdlog/spdlog.h>

namespace heterogeneous {

    namespace impl {
        template <typename PoolAlign = alignment::NoAlign, typename ItemAlign = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
        class BaseStackPool {
        private:
            virtual std::uint32_t fetch () const = 0;
            virtual std::uint32_t fetch_add (std::uint32_t) = 0;
            virtual void put (std::uint32_t) = 0;

            template <typename T>
            T* alloc () {
                return ItemAlign::template align<T>(unaligned_allocate(ItemAlign::adjust_size(sizeof(T))));
            }
        public:
            using PoolAlignType = PoolAlign;
            using ItemAlignType = ItemAlign;
            using OutOfSpacePolicyType = OutOfSpacePolicy;

            BaseStackPool (std::uint32_t size) :
                size(PoolAlign::adjust_size(size)),
                buffer(new std::byte[size]),
                base(PoolAlign::template align<std::byte>(buffer))
            {}
            BaseStackPool (BaseStackPool&& other) :
                size(other.size),
                buffer(other.buffer),
                base(other.base)
            {
                other.buffer = nullptr;
            }
            ~BaseStackPool() {
                if (buffer) {
                    delete [] buffer;
                }
            }

            // Allocate, but don't construct
            std::byte* unaligned_allocate (std::uint32_t bytes) {
                auto next = fetch_add(bytes);
                if (next + bytes <= size) {
                    std::byte* ptr = base + next;
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
                return new(alloc<T>()) T{item};
            }

            void reset () {
                put(0);
            }

            std::uint32_t remaining () const {
                return size - fetch();
            }

            std::uint32_t capacity () const {
                return size;
            }

            // Copy items from other into StackPool
            // AtomicStackPool WARNING: copy() is atomic, however `other` must not be reset while copy() is in progress. `other` may be added to, but this new memory will not be copied.
            template <typename SP>
            void pushAll (const BaseStackPool<typename SP::PoolAlignType, typename SP::ItemAlignType, typename SP::OutOfSpacePolicyType>& other) {
                auto other_next = other.fetch();
                auto next = fetch_add(other_next);
                if (next <= size) {
                    if (other_next > 0) { // Only copy if there is data to copy
                        std::memcpy(reinterpret_cast<void*>(base + next), reinterpret_cast<const void*>(other.base), other_next);
                    }
                } else {
                    OutOfSpacePolicy::template apply<void>("heterogeneous::StackPool");
                }
            }

            std::byte* begin () const {
                return base;
            }
            
            std::byte* end () const {
                return base + fetch();
            }

        private:
            const std::uint32_t size;
            std::byte* buffer;
            std::byte* const base;
        };
    }

    // A basic stack allocator. Objects can be allocated from the top of the stack, but are deallocated all at once. Pointers to elements are stable until reset() is called.
    template <typename PoolAlign = alignment::NoAlign, typename ItemAlign = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class StackPool : public impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy> {
    private:
        std::uint32_t fetch () const final { return next; }
        std::uint32_t fetch_add (std::uint32_t amount) final {
            auto cur = next;
            next += amount;
            return cur;
        }
        void put (std::uint32_t value) final { next = value; }
        std::uint32_t next;
    public:
        StackPool (std::uint32_t size) : impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy>(size), next(0) {}
        StackPool (StackPool&& other) : impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy>(std::move(other)), next(other.next) {}
        virtual ~StackPool() {}
    };

    // Same as StackPool, but uses an atomic next pointer allowing multiple threads to allocate objects from it concurrently. Pointers to elements are stable until reset() is called.
    template <typename PoolAlign = alignment::NoAlign, typename ItemAlign = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class AtomicStackPool : public impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy> {
    private:
        std::uint32_t fetch () const final { return next.load(); }
        std::uint32_t fetch_add (std::uint32_t amount) final { return next.fetch_add(amount); }
        void put (std::uint32_t value) final { next.store(value); }
        std::atomic_uint32_t next;
    public:
        AtomicStackPool (std::uint32_t size) : impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy>(size), next(0) {}
        AtomicStackPool (AtomicStackPool&& other) : impl::BaseStackPool<PoolAlign, ItemAlign, OutOfSpacePolicy>(std::move(other)), next(other.next) {}
        virtual ~AtomicStackPool() {}
    };
}