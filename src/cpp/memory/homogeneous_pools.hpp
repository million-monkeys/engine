#pragma once

#include "alignment.hpp"
#include "out_of_space_policies.hpp"

#include <type_traits>
#include <stdexcept>
#include <new>
#include <atomic>

namespace homogeneous {

    namespace impl {

        template <typename T, typename Align, typename OutOfSpacePolicy>
        class BaseStackPool {
        private:
            virtual std::uint32_t fetch () const = 0;
            virtual std::uint32_t fetch_add () = 0;
            virtual void put (std::uint32_t) = 0;

            // Allocate, but don't construct
            [[nodiscard]] T* allocate () {
                auto next = fetch_add();
                if (next < size) {
                    return pool + next;
                } else {
                    return OutOfSpacePolicy::template apply<T>("homogeneous::BaseStackPool");
                }
            }
        public:
            using Type = T;
            using AlignType = Align;
            using OutOfSpacePolicyType = OutOfSpacePolicy;

            BaseStackPool (std::uint32_t size) :
                memory(new std::byte[Align::adjust_size(sizeof(T) * size)]),
                pool(Align::template align<T>(memory)),
                size(size) {

            }
            BaseStackPool (BaseStackPool&& other) :
                memory(other.memory),
                pool(other.pool),
                size(other.size)
            {
                other.memory = nullptr;
            }
            virtual ~BaseStackPool() {
                if (memory) {
                    delete [] memory;
                }
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
                if constexpr (! std::is_trivial<T>::value) {
                    // Not a trivial type, so need to call the constructor
                    auto it = begin();
                    while (it != end()) {
                        it->~T();
                        ++it;
                    }
                }
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
            void pushAll (T* buffer, uint32_t count) {
                if (remaining() < count) {
                    OutOfSpacePolicy::template apply<void>("homogeneous::BaseStackPool");
                }
                // TODO: benchmark copy_n, copy, memmove and memcpy
                std::copy_n(buffer, count, end());
                // std::memcpy(reinterpret_cast<void*>(pool + next), reinterpret_cast<const void*>(buffer), sizeof(T) * count);
                fetch_add(count);
            }

            // Copy items from other into StackPool
            template <typename PT>
            void pushAll (BaseStackPool<typename PT::Type, typename PT::AlignType, typename PT::OutOfSpacePolicyType>& other) {
                pushAll(other.pool, other.count());
            }

        private:
            std::byte* const memory;
            T* const pool;
            const std::uint32_t size;
        };

    }


    // A basic stack allocator. Objects can be allocated from the top of the stack, but are deallocated all at once. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class StackPool : public impl::BaseStackPool<T, Align, OutOfSpacePolicy> {
    private:
        std::uint32_t fetch () const final { return next; }
        std::uint32_t fetch_add () final { return next++; }
        void put (std::uint32_t value) final { next = value; }
        std::uint32_t next;
    public:
        StackPool (std::uint32_t size) : impl::BaseStackPool<T, Align, OutOfSpacePolicy>(size), next(0) {}
        StackPool (StackPool&& other) : impl::BaseStackPool<T, Align, OutOfSpacePolicy>(std::move(other)), next(other.next) {}
        virtual ~StackPool() {}
    };


    // Same as StackPool, but uses an atomic next pointer allowing multiple threads to allocate objects from it concurrently. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class AtomicStackPool : public impl::BaseStackPool<T, Align, OutOfSpacePolicy> {
    private:
        std::uint32_t fetch () const final { return next.load(); }
        std::uint32_t fetch_add () final { return next.fetch_add(1); }
        void put (std::uint32_t value) final { next.store(value); }
        std::atomic_uint32_t next;
    public:
        AtomicStackPool (std::uint32_t size) : impl::BaseStackPool<T, Align, OutOfSpacePolicy>(size), next(0) {}
        AtomicStackPool (AtomicStackPool&& other) : impl::BaseStackPool<T, Align, OutOfSpacePolicy>(std::move(other)), next(other.next) {}
        virtual ~AtomicStackPool() {}
    };


    // A free-list based pool. Objects can be allocated and deallocated, unused space will be reused. Pointers to elements are stable until reset() is called.
    template <typename T, typename Align = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class Pool {
    private:
        [[nodiscard]] T* allocate () {
            if (next != nullptr) {
                auto item = next;
                next = next->next;
                --free;
                return &item->object;
            } else {
                return OutOfSpacePolicy::template apply<T>("homogeneous::Pool");
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
        Pool (Pool&& other) :
            memory(other.memory),
            pool(other.pool),
            next(other.next),
            free(other.free),
            size(other.size)
        {
            other.memory = nullptr;
        }

        ~Pool() {
            if (memory) {
                delete [] memory;
            }
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
        std::byte* memory;
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

    
    namespace detail {
        constexpr int roundUp (int n) {
            for (auto i : {8, 16, 32, 64}) {
                if (n < i) return i;
            }
            return 0;
        }
        template <int N>
        struct BitsetUnderlying {
            static_assert(N != 0, "Size must be greater than 0 and less than or equal to 64");
            using Type = std::uint8_t;
        };
        template <> struct BitsetUnderlying<8>  { using Type = std::uint8_t;  };
        template <> struct BitsetUnderlying<16> { using Type = std::uint16_t; };
        template <> struct BitsetUnderlying<32> { using Type = std::uint32_t; };
        template <> struct BitsetUnderlying<64> { using Type = std::uint64_t; };
    }

    // A pool using a bitset to determine which items are free
    template <typename T, std::size_t N, typename Align = alignment::NoAlign, typename OutOfSpacePolicy = out_of_space_policies::Throw>
    class BitsetPool {
    private:
            // Allocate, but don't construct
            [[nodiscard]] T* allocate () {
                if (bitset & calcResetValue(N)) {
                    // replace __builtin_ctz with std::countr_zero(x) from <bits> when switching to C++20
                    unsigned bit = __builtin_ctz(bitset);
                    T* ptr = (pool + bit);
                    bitset ^= (1 << bit);
                    return ptr;
                } else {
                    return OutOfSpacePolicy::template apply<T>("homogeneous::BaseStackPool");
                }
            }

            constexpr unsigned calcResetValue (unsigned n) {
                unsigned value = 0;
                do {
                    value = (value << 1) | 1;
                } while (--n);
                return value;
            }
    public:
        BitsetPool () :
            memory(new std::byte[Align::adjust_size(sizeof(T) * N)]),
            pool(Align::template align<T>(memory)),
            bitset(calcResetValue(N))
        {
        }
        ~BitsetPool () { delete [] memory; }

        template <typename... Args>
        [[nodiscard]] T* emplace (Args&&... args) {
            return new(allocate()) T{args...};
        }

        [[nodiscard]] T* insert (const T& other) {
            return new(allocate()) T(other);
        }

        void discard (T* object) {
            // Not a trivial type, so need to call the constructor
            if constexpr (! std::is_trivial<T>::value) {
                object->~T();
            }
            bitset |= (1 << (object - pool) / sizeof(T));
        }

        void reset () {
            // Not a trivial type, so need to call the constructor
            if constexpr (! std::is_trivial<T>::value) {
                T* ptr = pool;
                unsigned bit = N;
                do {
                    --bit;
                    if (! (bitset >> bit) & 1) {
                        ptr->~T();
                    }
                    ++ptr;
                } while (bit);
            }
            bitset = calcResetValue(N);
        }

        std::uint32_t remaining () const {
            return N - count();
        }

        std::uint32_t count () const {
            std::uint32_t c = 0;
            unsigned bit = N;
            do {
                --bit;
                c += 1 - (bitset >> bit) & 1;
            } while (bit);
            return c;
        }

        std::uint32_t capacity () const {
            return N;
        }

    private:
        std::byte* const memory;
        T* const pool;
        typename detail::BitsetUnderlying<detail::roundUp(N)>::Type bitset;
    };

}
