#pragma once

#include <mutex>
#include <condition_variable>

struct ImGuiContext;

namespace core {
    class Engine;
}

namespace gou::api {
    class Renderer;
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
    void waitForRenderer ();


    gou::api::Renderer* init (core::Engine&, graphics::Sync*&, ImGuiContext*&);
    void windowChanged(gou::api::Renderer*);
    void term (gou::api::Renderer*);

} // graphics::
