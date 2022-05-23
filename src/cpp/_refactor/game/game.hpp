#pragma once

#include <monkeys.hpp>
#include "_refactor/utils/timekeeping.hpp"

namespace game {
    Context* init (events::Context* events_ctx, messages::Context* messages_ctx, world::Context* world_ctx, scripting::Context* scripting_ctx, resources::Context* resources_ctx);
    void term (Context* context);

    void setState (Context* context, entt::hashed_string new_state);

    void execute (Context* context, timing::Time current_time, timing::Delta delta, uint64_t frame_count);
    void executeHandlers (Context* context);
}