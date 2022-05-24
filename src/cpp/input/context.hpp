#pragma once

#include <monkeys.hpp>

#include <SDL.h>

namespace input {
    struct Context {
        Context (million::events::Stream& commands) : m_commands(commands) {}
        ~Context () {}

        million::events::Stream& m_commands;
        SDL_GameController* m_game_controller;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Lime900,
            profiler::colors::Lime700,
            profiler::colors::Lime500,
            profiler::colors::Lime300,
            profiler::colors::Lime100,
        };
        return colors[idx];
    }
}
