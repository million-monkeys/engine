#pragma once

#include <monkeys.hpp>

struct GameEventHandler {
    entt::hashed_string::hash_type events;
    million::GameHandler handler;
};

namespace game {
    struct Context {
        Context (million::events::Stream& stream, million::events::Stream& commands) : m_stream(stream), m_commands(commands) {}
        ~Context () {}

        events::Context* m_events_ctx;
        messages::Context* m_messages_ctx;
        world::Context* m_world_ctx;
        scripting::Context* m_scripting_ctx;
        resources::Context* m_resources_ctx;
        scheduler::Context* m_scheduler_ctx;
        physics::Context* m_physics_ctx;

        million::events::Stream& m_stream;   // "game"_hs output stream
        million::events::Stream& m_commands;

        // Timing
        timing::Delta m_current_time_delta = 0;
        timing::Time m_current_time = 0;
        uint64_t m_current_frame = 0;

        // Game state
        entt::hashed_string::hash_type m_current_state;    // Track whether on menu, loading screen, etc

        // Game event handlers
        helpers::hashed_string_flat_map<std::vector<GameEventHandler>> m_game_handlers;

        // Game scripts
        helpers::hashed_string_flat_map<million::resources::Handle> m_game_scripts;
        million::resources::Handle m_current_game_script; // Cache the active one to avoid a hash map lookup every frame
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::BlueGrey900,
            profiler::colors::BlueGrey700,
            profiler::colors::BlueGrey500,
            profiler::colors::BlueGrey300,
            profiler::colors::BlueGrey100,
        };
        return colors[idx];
    }
}
