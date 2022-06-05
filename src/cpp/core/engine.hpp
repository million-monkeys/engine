#pragma once

#include <monkeys.hpp>

class Engine {
public:
    bool init (std::shared_ptr<spdlog::logger> logger);
    void execute ();
    void shutdown ();

private:
    events::Context* m_events_ctx = nullptr;
    messages::Context* m_messages_ctx = nullptr;
    modules::Context* m_modules_ctx = nullptr;
    resources::Context* m_resources_ctx = nullptr;
    input::Context* m_input_ctx = nullptr;
    scripting::Context* m_scripting_ctx = nullptr;
    world::Context* m_world_ctx = nullptr;
    game::Context* m_game_ctx = nullptr;
    scheduler::Context* m_scheduler_ctx = nullptr;
    graphics::Context* m_graphics_ctx = nullptr;

    [[maybe_unused]] audio::Context* m_audio_ctx = nullptr;
    
    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Indigo900,
            profiler::colors::Indigo700,
            profiler::colors::Indigo500,
            profiler::colors::Indigo300,
            profiler::colors::Indigo100,
        };
        return colors[idx];
    }

    bool handle_commands ();
};