#pragma once

#include <cstdint>

namespace alignment {
    struct NoAlign {
        static std::uint32_t adjust_size (std::uint32_t size) { return size; }
        template <typename T> static T* align (void* buffer) {
            return reinterpret_cast<T*>(buffer);
        }
    };

    template <int BoundaryT>
    struct Aligned {
        static constexpr int Bountary = BoundaryT;
        static std::uint32_t adjust_size (std::uint32_t size) { return size + BoundaryT; }
        template <typename T> static T* align (void* buffer) {
            return reinterpret_cast<T*>(helpers::align(buffer, BoundaryT));
        }
    };

    using AlignCacheLine = Aligned<64>;
    using AlignSIMD= Aligned<16>;
}