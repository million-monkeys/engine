#pragma once

#include <game.hpp>

namespace core {
    class Engine;
}

namespace resources {

    void init ();
    void poll (core::Engine*);

    void install (entt::id_type id, monkeys::api::resources::Loader* loader);
    template <typename T> void install () { install(entt::type_index<T>::value(), new T()); }
    
    monkeys::resources::Handle load (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name);
    void term ();

}
