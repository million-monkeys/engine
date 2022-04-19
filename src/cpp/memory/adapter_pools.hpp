#pragma once

namespace adapter {
    
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

}