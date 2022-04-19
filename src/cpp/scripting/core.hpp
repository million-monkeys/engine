#pragma once

#include <game.hpp>

namespace core {
    class Engine;
}

namespace scripting {
    bool init (core::Engine* engine);
    void registerComponent (entt::hashed_string::hash_type name, entt::id_type id);
    [[maybe_unused]] bool load (const std::string& filename);
    void term ();
}
