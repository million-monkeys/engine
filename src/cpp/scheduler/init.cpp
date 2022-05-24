#include "scheduler.hpp"
#include "context.hpp"

scheduler::Context* scheduler::init (world::Context* world_ctx, scripting::Context* scripting_ctx, physics::Context* physics_ctx, events::Context* events_ctx, game::Context* game_ctx, modules::Context* modules_ctx)
{
    auto context = new scheduler::Context;
    context->m_world_ctx = world_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_physics_ctx = physics_ctx;
    context->m_events_ctx = events_ctx;
    context->m_game_ctx = game_ctx;
    context->m_modules_ctx = modules_ctx;

    context->m_system_status = scheduler::SystemStatus::Stopped;
    return context;
}

void scheduler::term (scheduler::Context* context)
{
    context->m_coordinator.clear();
    for (auto&& [_, taskflow] : context->m_systems) {
        delete taskflow;
    }
    delete context;
}