#pragma once

#include <game.hpp>

namespace core {
    class Engine;
}

namespace resources {

    void init (core::Engine*);
    void poll ();

    void install (entt::id_type id, million::api::resources::Loader* loader);
    template <typename T> void install (core::Engine* engine) { install(entt::type_index<T>::value(), new T(engine)); }
    
    million::resources::Handle load (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name);
    void term ();

}
