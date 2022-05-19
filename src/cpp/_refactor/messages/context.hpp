#pragma once

#include <game.hpp>
#include "_refactor/memory/event_pools.hpp"

namespace messages {
    struct Context {
        Context ();
        ~Context () {}
        std::vector<memory::MessagePool*> m_message_pools;
        memory::MessagePool::PoolType m_message_pool;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::DeepPurple900,
            profiler::colors::DeepPurple700,
            profiler::colors::DeepPurple500,
            profiler::colors::DeepPurple300,
            profiler::colors::DeepPurple100,
        };
        return colors[idx];
    }
}
