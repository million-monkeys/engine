#include "scheduler.hpp"
#include "context.hpp"

int get_num_workers () {
    auto max_workers = std::thread::hardware_concurrency();
    // Keep some cores free for rendeder, resource loader and audio, but if not enough cores are available then use all available
    // Kept free: 2 cores if 4 or more cores are available
    //            1 cores if 2 or 3 cores  are available
    // Otherwise no cores are kept free and the core must be shared
    // Recommended to have at least 4 cores
    if (max_workers >= 4) {
        return max_workers - 2;
    } else if (max_workers >= 2) {
        return max_workers - 1;
    } else {
        return max_workers;
    }
}

scheduler::Context* scheduler::init (world::Context* world_ctx, scripting::Context* scripting_ctx, events::Context* events_ctx, game::Context* game_ctx, modules::Context* modules_ctx)
{
    EASY_BLOCK("scheduler::init", scheduler::COLOR(1));
    SPDLOG_DEBUG("[scheduler] Init");
    auto context = new scheduler::Context{get_num_workers()};
    context->m_world_ctx = world_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_events_ctx = events_ctx;
    context->m_game_ctx = game_ctx;
    context->m_modules_ctx = modules_ctx;

    context->m_timestep_cccumulator = 0.0f;
    context->m_step_size = 1.0f / 60.0f;
    context->m_frames_late = 0;

    context->m_system_status = scheduler::SystemStatus::Stopped;
    return context;
}

void scheduler::term (scheduler::Context* context)
{
    EASY_BLOCK("scheduler::term", scheduler::COLOR(1));
    SPDLOG_DEBUG("[scheduler] Term");
    context->m_coordinator.clear();
    for (auto&& [_, taskflow] : context->m_systems) {
        delete taskflow;
    }
    delete context;
}