#pragma once

#include "utils/helpers.hpp"

#include <type_traits>
#include <stdexcept>
#include <new>
#include <atomic>

namespace memory {

    struct NoAlign {
        static uint32_t adjust_size (std::uint32_t size) { return size; }
        template <typename T> static T* align (void* buffer) {
            return reinterpret_cast<T*>(buffer);
        }
    };

    template <int Boundary>
    struct Aligned {
        static uint32_t adjust_size (std::uint32_t size) { return size + Boundary; }
        template <typename T> static T* align (void* buffer) {
            return reinterpret_cast<T*>(helpers::align(reinterpret_cast<void*>(buffer), Boundary));
        }
    };

    using AlignCacheLine = Aligned<64>;
    using AlignSIMD= Aligned<16>;


    template <typename T, typename Align = NoAlign>
    class StackPool {
    public:
        static_assert(std::is_trivial<T>::value, "StackPool<T> must contain a trivial type");
        using Type = T;
        using AlignType = Align;

        StackPool (std::uint32_t size) :
            memory(new std::byte[Align::adjust_size(sizeof(T) * size)]),
            pool(Align::template align<T>(memory)),
            next(0),
            size(size) {

        }
        ~StackPool() {
            delete [] memory;
        }

        // Allocate, but don't construct
        T* allocate () {
            if (next < size) {
                return (pool + next++);
            } else {
                throw std::runtime_error("StackPool allocated more items than reserved space");
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
            std::memcpy(reinterpret_cast<void*>(pool + next++), reinterpret_cast<const void*>(item), sizeof(T));
        }

        void reset () {
            next = 0;
        }

        std::uint32_t remaining () const {
            return size - next;
        }

        std::uint32_t count () const {
            return next;
        }

        std::uint32_t capacity () const {
            return size;
        }

        T* begin () {
            return pool;
        }

        T* end () {
            return pool + next;
        }

        const T* cbegin () const {
            return pool;
        }

        const T* cend () const {
            return pool + next;
        }

        // Copy buffer into StackPool
        void copy (T* buffer, uint32_t count) {
            if (remaining() < count) {
                throw std::runtime_error("StackPool attempted to copy more elements than remaining space allows");
            }
            // TODO: benchmark copy_n, copy, memmove and memcpy
            std::copy_n(buffer, count, end());
            // std::memcpy(reinterpret_cast<void*>(pool + next), reinterpret_cast<const void*>(buffer), sizeof(T) * count);
            next += count;
        }

        // Copy items from other into StackPool
        template <typename PT>
        void copy (StackPool<typename PT::Type, typename PT::AlignType>& other) {
            copy(other.pool, other.count());
        }

    private:
        std::byte* const memory;
        T* const pool;
        std::uint32_t next;
        const std::uint32_t size;
    };


    template <typename T, typename Align = NoAlign>
    class AtomicStackPool {
    public:
        static_assert(std::is_trivial<T>::value, "AtomicStackPool<T> must contain a trivial type");
        using Type = T;
        using AlignType = Align;

        AtomicStackPool (std::uint32_t size) :
            memory(new std::byte[Align::adjust_size(sizeof(T) * size)]),
            pool(Align::template align<T>(memory)),
            next(0),
            size(size) {

        }
        ~AtomicStackPool() {
            delete [] memory;
        }

        // Allocate, but don't construct
        T* allocate () {
            if (next < size) {
                return (pool + next++);
            } else {
                throw std::runtime_error("AtomicStackPool allocated more items than reserved space");
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
            std::memcpy(reinterpret_cast<void*>(pool + next++), reinterpret_cast<const void*>(item), sizeof(T));
        }

        void reset () {
            next.store(0);
        }

        std::uint32_t remaining () const {
            return size - next.load();
        }

        std::uint32_t count () const {
            return next.load();
        }

        std::uint32_t capacity () const {
            return size;
        }

        T* begin () {
            return pool;
        }

        T* end () {
            return pool + next.load();
        }

        const T* cbegin () const {
            return pool;
        }

        const T* cend () const {
            return pool + next.load();
        }

        // Copy buffer into StackPool
        void copy (T* buffer, uint32_t count) {
            if (remaining() < count) {
                throw std::runtime_error("AtomicStackPool attempted to copy more elements than remaining space allows");
            }
            // TODO: benchmark copy_n, copy, memmove and memcpy
            std::copy_n(buffer, count, end());
            // std::memcpy(reinterpret_cast<void*>(pool + next), reinterpret_cast<const void*>(buffer), sizeof(T) * count);
            next.fetch_add(count);
        }

        // Copy items from other into StackPool
        template <typename PT>
        void copy (StackPool<typename PT::Type, typename PT::AlignType>& other) {
            copy(other.pool, other.count());
        }

    private:
        std::byte* const memory;
        T* const pool;
        std::atomic_uint32_t next;
        const std::uint32_t size;
    };


    template <typename T, typename Align = NoAlign>
    class Pool {
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
        [[nodiscard]] T* allocate (Args... args) {
            if (next != nullptr) {
                auto item = next;
                next = next->next;
                --free;
                return &item->object;
            } else {
                throw std::runtime_error("Pool allocated more items than reserved space");
            }
        }

        template <typename... Args>
        [[nodiscard]] T* emplace (Args&&... args) {
            return new(allocate()) T{args...};
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


    template <typename T, typename Allocator = std::allocator<T>>
    class ReorderingPool {
    public:
        static_assert(std::is_trivial<T>::value, "ReorderingPool<T> must contain a trivial type");
        using Type = T;

        ReorderingPool (uint32_t size) {
            pool.reserve(size);
        }

        template <typename... Args>
        [[nodiscard]] T* push (Args... args) {
            pool.push_back(T{args...});
            return &pool.back();
        }

        template <typename... Args>
        [[nodiscard]] T* emplace (Args&&... args) {
            return pool.emplace_back(std::forward<Args>(args)...);
        }

        void discard (T* object) {
            auto index = helpers::binary_search(pool, object);
            helpers::remove(pool, index);
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


    template <typename PoolType>
    class DoubleBuffered {
    public:
        DoubleBuffered (uint32_t size) :
            pools{PoolType{size}, PoolType{size}},
            index(0) {

        }

        template <typename... Args>
        [[nodiscard]] auto emplace (Args&&... args) {
            return pools[index].emplace(std::forward<Args>(args)...);
        }    
        void discard (typename PoolType::Type* object) {
            pools[index].discard(object);
        }

        void swap () {
            index = 1 - index;
            pools[index].reset();
        }

        void reset () {
            pools[index].reset();
        }

        std::uint32_t count () const {
            return pools[index].count();
        }

        std::uint32_t remaining () const {
            return pools[index].remaining();
        }

        std::uint32_t capacity () const {
            return pools[index].capacity();
        }

        PoolType& front() {
            return pools[index];
        }

        PoolType& back() {
            return pools[1 - index];
        }

        const PoolType& cfront() const {
            return pools[index];
        }

        const PoolType& cback() const {
            return pools[1 - index];
        }
    private:
        PoolType pools[2];
        std::uint32_t index;
    };

} // memory::
