#pragma once

#include <monkeys.hpp>
#include <mutex>
#include <condition_variable>

namespace graphics {
    struct Sync {
        std::mutex state_mutex;
        std::condition_variable sync_cv;
        enum class Owner {
            Engine,
            Renderer,
        } owner = Owner::Engine;
    };

    Context* init (world::Context* world_ctx, input::Context* input_ctx, modules::Context* modules_ctx);
    void term (Context* context);
    void handOff (Context* context);
}
