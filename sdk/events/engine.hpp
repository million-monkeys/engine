#pragma once

#include <million/types.hpp>
#include <million/macros.hpp>

namespace events::engine {

    DECLARE_EVENT(LoadScene, "scene/load") {
        entt::hashed_string::hash_type scene_id;
        bool auto_swap;
    };

    DECLARE_EVENT(ResourceLoaded, "resource/loaded") {
        entt::hashed_string::hash_type type;
        entt::hashed_string::hash_type name;
        million::resources::Handle handle;
    };

}
