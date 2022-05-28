#pragma once

#include <monkeys.hpp>
#include "utils/timekeeping.hpp"
#include "scheduler/scheduler.hpp"

namespace game {
    Context* init (events::Context* events_ctx, messages::Context* messages_ctx, world::Context* world_ctx, scripting::Context* scripting_ctx, resources::Context* resources_ctx, modules::Context* modules_ctx);
    void term (Context* context);
    void setScheduler (Context* context, scheduler::Context* scheduler_ctx);
    std::optional<scheduler::SystemStatus> setup (Context* context);

    void registerHandler (Context* context, entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler);
    void setState (Context* context, entt::hashed_string new_state);

    void execute (Context* context, timing::Time current_time, timing::Delta delta, uint64_t current_frame);
    void executeHandlers (Context* context);

    timing::Delta deltaTime (Context* context);
    timing::Time currentTime (Context* context);
    uint64_t currentFrame (Context* context);
}