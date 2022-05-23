#pragma once

#include <monkeys.hpp>

namespace physics {
    Context* init ();
    void term (Context* context);

    void prepare (Context* context, entt::registry& registry);
    void simulate (Context* context);
}