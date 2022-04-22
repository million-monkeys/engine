#pragma once

#include <game.hpp>

namespace resources {

    class Resource {
    public:
        virtual bool load (entt::hashed_string::hash_type id, const std::string& filename) = 0;
        virtual void unload (entt::hashed_string::hash_type id) = 0;
    };

}
