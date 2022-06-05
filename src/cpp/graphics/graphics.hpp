#pragma once

#include <monkeys.hpp>
#include <mutex>
#include <condition_variable>

namespace graphics {
    Context* init (world::Context* world_ctx, input::Context* input_ctx, modules::Context* modules_ctx);
    bool init_ok (Context* context);
    void term (Context* context);
    void handOff (Context* context);
}
