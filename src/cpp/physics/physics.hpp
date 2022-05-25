#pragma once

#include <monkeys.hpp>

namespace physics {
    Context* init (world::Context* world_ctx);
    void term (Context* context);

    void createScene (Context* context);

    void prepare (Context* context, entt::registry& registry);
    void simulate (Context* context, float delta);
}