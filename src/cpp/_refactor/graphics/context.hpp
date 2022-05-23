#pragma once

#include <monkeys.hpp>
#include "graphics.hpp"
#include <thread>

namespace graphics {
    struct Context {
        world::Context* m_world_ctx;
        input::Context* m_input_ctx;

        Sync m_sync;
        std::thread m_graphics_thread;
        std::atomic_bool m_running = false;
        std::atomic_bool m_initialized = false;
        std::atomic_bool m_error = false;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Orange900,
            profiler::colors::Orange700,
            profiler::colors::Orange500,
            profiler::colors::Orange300,
            profiler::colors::Orange100,
        };
        return colors[idx];
    }
}
