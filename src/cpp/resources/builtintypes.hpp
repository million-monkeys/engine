#pragma once

#include "resources.hpp"

namespace resources::types {

    class EntityScripts : public million::api::resources::Loader {
    public:
        virtual ~EntityScripts() {}
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "entity-script"_hs;
    };

    class SceneScripts : public million::api::resources::Loader {
    public:
        virtual ~SceneScripts() {}
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-script"_hs;
    };

    class SceneEntities : public million::api::resources::Loader {
    public:
        virtual ~SceneEntities() {}
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-entities"_hs;
    };

}
