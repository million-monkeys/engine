#pragma once

#include <monkeys.hpp>

namespace physics {
    Context* init (modules::Context* modules_ctx, world::Context* world_ctx);
    void term (Context* context);

    void createScene (Context* context);

    void simulate (Context* context, timing::Time current, timing::Delta delta);
}