#pragma once

#include <million/types.hpp>
#include <million/macros.hpp>

namespace commands {
    namespace engine {
        DECLARE_EVENT(Exit, "engine/exit") {};
    }

    namespace scene {
        DECLARE_EVENT(Load, "scene/load") {
            entt::hashed_string::hash_type scene_id;
            bool auto_swap;
        };
    }
}

namespace events {

    namespace resources {
        DECLARE_EVENT(Loaded, "resource/loaded") {
            entt::hashed_string::hash_type type;
            entt::hashed_string::hash_type name;
            million::resources::Handle handle;
        };
    }

    namespace scene {
        DECLARE_EVENT(Loaded, "scene/loaded") {
            entt::hashed_string::hash_type id;
        };

        DECLARE_EVENT(Activated, "scene/activated") {
            entt::hashed_string::hash_type id;
        };
    }

}
