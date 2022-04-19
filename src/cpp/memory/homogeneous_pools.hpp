#pragma once

#include "alignment.hpp"

#include "utils/helpers.hpp"

#include <type_traits>
#include <stdexcept>
#include <new>
#include <atomic>

namespace homogeneous {

    namespace impl {

        template <typename NextT, typename T, typename Align = alignment::NoAlign>
        class BaseStackPool {
        private:
            virtual std::uint32_t fetch () const = 0;
            virtual std::uint32_t fetch_add () const = 0;
            virtual void put (std::uint32_t) = 0;

            // Allocate, but don't construct
            [[nodiscard]] T* allocate () {
                if (fetch() < size) {
                    return (pool + fetch_add());
                } else {
                    throw std::runtime_error("StackPool allocated more items than reserved space");
                }
            }
        public:
            using Type = T;
            using AlignType = Align;

            BaseStackPool (std::uint32_t size) :
                memory(new std::byte[Align::adjust_size(sizeof(T) * size)]),
                pool(Align::template align<T>(memory)),
                size(size),
                next(0) {

            }
            virtual ~BaseStackPool() {
                delete [] memory;
            }
                
            // Allocate and construct
            template <typename... Args>
            T* emplace (Args&&... args) {
                return new(allocate()) T{args...};
            }

            void discard (T* object) const {
            }

            void push_back (const T& item) {
                std::memcpy(reinterpret_cast<void*>(pool + fetch_add()), reinterpret_cast<const void*>(item), sizeof(T));
            }

            void reset () {
                put(0);
            }

            std::uint32_t remaining () const {
                return size - fetch();
            }

            std::uint32_t count () const {
                return fetch();
            }

            std::uint32_t capacity () const {
                return size;
            }

            T* begin () {
                return pool;
            }

            T* end () {
                return pool + fetch();
            }

            const T* cbegin () const {
                return pool;
            }

            const T* cend () const {
                return pool + fetch();
            }

            // Copy buffer into StackPool
            void copy (T* buffer, uint32_t count) {
                if (remaining() < count) {
                    throw std::runtime_error("StackPool attempted to copy more elements than remaining space allows");
                }
                // TODO: benchmark copy_n, copy, memmove and memcpy
                std::copy_n(buffer, count, end());
                // std::memcpy(reinterpret_cast<void*>(pool + next), reinterpret_cast<const void*>(buffer), sizeof(T) * count);
                fetch_add(count);
            }

            // Copy items from other into StackPool
            template <typename PT>
            void copy (StackPool<typename PT::Type, typename PT::AlignType>& other) {
                copy(other.pool, other.count());
            }

        private:
            std::byte* const memory;
            T* const pool;
            const std::uint32_t size;
        protected:
            NextT next;
        };

    }


    // A basic stack allocator. Objects can be allocated from the top of the stack, but are deallocated all at once. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign>
    class StackPool : public impl::StackPool<std::uint32_t, Align> {
    private:
        std::uint32_t fetch () final { return next; }
        std::uint32_t fetch_add () final { return next++; }
        void put (std::uint32_t value) final { next = value; }
    public:
        static_assert(std::is_trivial<T>::value, "StackPool<T> must contain a trivial type");
        StackPool (std::uint32_t size) : BaseStackPool(size) {}
        virtual ~StackPool() {}
    };


    // Same as StackPool, but uses an atomic next pointer allowing multiple threads to allocate objects from it concurrently. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign>
    class AtomicStackPool : public impl::StackPool<std::atomic_uint32_t, Align> {
    private:
        std::uint32_t fetch () final { return next.load(); }
        std::uint32_t fetch_add () final { return next.fetch_add()++; }
        void put (std::uint32_t value) final { next.set(value); }
    public:
        static_assert(std::is_trivial<T>::value, "AtomicStackPool<T> must contain a trivial type");
        AtomicStackPool (std::uint32_t size) : BaseStackPool(size) {}
        virtual ~AtomicStackPool() {}
    };


    // A free-list based pool. Objects can be allocated and deallocated, unused space will be reused. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign>
    class Pool {
    private:
        [[nodiscard]] T* allocate () {
            if (next != nullptr) {
                auto item = next;
                next = next->next;
                --free;
                return &item->object;
            } else {
                throw std::runtime_error("Pool allocated more items than reserved space");
            }
        }
    public:
        static_assert(std::is_trivial<T>::value, "Pool<T> must contain a trivial type");
        using Type = T;

        Pool (std::uint32_t size) :
            memory(new std::byte[Align::adjust_size(sizeof(T) * size)]),
            pool(reinterpret_cast<Item*>(Align::template align<T>(memory))),
            size(size) {
            reset();
        }

        ~Pool() {
            delete [] memory;
        }

        template <typename... Args>
        [[nodiscard]] T* emplace (Args&&... args) {
            return new(allocate()) T{args...};
        }

        [[nodiscard]] T* insert (const T& other) {
            return new(allocate()) T(other);
        }

        void discard (T* object) {
            std::uint64_t addr = reinterpret_cast<std::uint64_t>(object);
            std::uint64_t first =  reinterpret_cast<std::uint64_t>(pool);
            if (addr < first || addr > first + (sizeof(Item) * size)) {
                throw std::runtime_error("Pool discarded object not belonging to pool");
            }
            Item* item = reinterpret_cast<Item*>(object);
            item->next = next;
            next = item;
            ++free;
        }

        void reset () {
            for (Item* item = pool; item < pool + size; ++item) {
                item->next = item + 1;
            }
            pool[size-1].next = nullptr;
            next = pool;
            free = size;
        }

        uint32_t count () const {
            return size - free;
        }

        uint32_t remaining () const {
            return free;
        }

        uint32_t capacity () const {
            return size;
        }

    private:
        std::byte* const memory;
        union Item {
            T object;
            Item* next;
        };
        Item* const pool;
        Item* next;
        std::uint32_t free;
        const std::uint32_t size;
    };


    // A pool maintaining tightly packed elements. Discarded elements are swapped to the back. Pointers to elements are NOT stable.
    template <typename T, typename Allocator = std::allocator<T>>
    class ReorderingPool {
    public:
        static_assert(std::is_trivial<T>::value, "ReorderingPool<T> must contain a trivial type");
        using Type = T;

        ReorderingPool (uint32_t size) {
            pool.reserve(size);
        }

        template <typename... Args>
        [[nodiscard]] T* emplace (Args&&... args) {
            return pool.emplace_back(std::forward<Args>(args)...);
        }

        [[nodiscard]] T* insert (const T& other) {
            pool.push_back(T{other});
            return &pool.back();
        }

        void discard (T* object) {
            if (object >= pool.data() && object < pool.data() + pool.size()) {
                auto index = helpers::binary_search(pool, object);
                helpers::remove(pool, index);
            }
        }

        void reset () {
            pool.clear();
        }

        std::uint32_t count () const {
            return pool.size();
        }

        std::uint32_t remaining () const {
            return pool.capacity() - pool.size();
        }

        std::uint32_t capacity () const {
            return pool.capacity();
        }

        typename std::vector<T>::iterator begin () {
            return pool.begin();
        }

        typename std::vector<T>::iterator end () {
            return pool.end();
        }

        typename std::vector<T>::const_iterator cbegin () const {
            return pool.cbegin();
        }

        typename std::vector<T>::const_iterator cend () const {
            return pool.cend();
        }

    private:
        std::vector<T, Allocator> pool;
    };
}
