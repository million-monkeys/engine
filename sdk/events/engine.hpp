#pragma once

#include <million/types.hpp>
#include <million/macros.hpp>

namespace events::engine {

    DECLARE_EVENT(LoadScene, "scene/load") {
        entt::hashed_string scene_id;
    };

    DECLARE_EVENT(ResourceLoaded, "resource/loaded") {
        entt::hashed_string::hash_type name;
        monkeys::resources::Handle handle;
    };

}
