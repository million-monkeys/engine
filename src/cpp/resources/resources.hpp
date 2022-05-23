#pragma once

#include <monkeys.hpp>

namespace core {
    class Engine;
}

namespace resources {

    void init (core::Engine*);
    void poll ();

    void install (million::api::resources::Loader* loader, bool managed=false);
    template <typename T> void install (core::Engine* engine) { install(new T(engine), true); }
    
    million::resources::Handle load (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name);
    void term ();

}
