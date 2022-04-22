#pragma once

#include <game.hpp>

namespace core {
    class Engine;
}

namespace scripting {

    bool init (core::Engine& engine);
    void registerComponent (entt::hashed_string::hash_type name, entt::id_type id);
    bool evaluate (const std::string& name, const std::string& source);
    bool load (const std::string& filename);
    void processEvents (monkeys::api::Engine& engine);
    void term ();

}
