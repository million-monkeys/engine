#pragma once

#include <game.hpp>

namespace core {
    class Engine;
}
struct lua_State;

namespace scripting {

    class Engine {
    public:
        Engine (core::Engine* engine);
        ~Engine ();

        bool init ();
        void registerComponent (entt::hashed_string::hash_type name, entt::id_type id);
        [[maybe_unused]] bool load (const std::string& filename);
        void term ();

    private:
        core::Engine* m_engine;
        lua_State* m_lua_state;
    };
}
