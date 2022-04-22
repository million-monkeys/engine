#pragma once

#include "manager.hpp"

class ScriptedEvents : resources::Resource {
public:
    bool load (entt::hashed_string::hash_type id, const std::string& filename);
    void unload (entt::hashed_string::hash_type id);
};
