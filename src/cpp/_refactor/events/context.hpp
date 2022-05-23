#pragma once

#include <monkeys.hpp>

#include "_refactor/memory/event_pools.hpp"

struct StreamInfo {
    memory::IterableStream* iterable;
    million::events::Stream* streamable;
};

namespace events {
    struct Context {
        Context ();
        ~Context () {}

        million::events::Stream& m_commands;
        helpers::hashed_string_node_map<StreamInfo> m_named_streams; // TODO: delete
        helpers::hashed_string_node_map<StreamInfo> m_engine_streams;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Purple900,
            profiler::colors::Purple700,
            profiler::colors::Purple500,
            profiler::colors::Purple300,
            profiler::colors::Purple100,
        };
        return colors[idx];
    }
}
