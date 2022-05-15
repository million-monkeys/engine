#pragma once

#include <game.hpp>

#include <mutex>
#include <condition_variable>

namespace core {
    class Engine;
}

namespace graphics {

    struct Sync {
        std::mutex state_mutex;
        std::condition_variable sync_cv;
        enum class Owner {
            Engine,
            Renderer,
        } owner = Owner::Engine;
    };

    Sync* init (core::Engine&);
    void term (Sync*);

}