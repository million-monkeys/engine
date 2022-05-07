#pragma once

#include "resources.hpp"
#include <game.hpp>

namespace core {
    class Engine;
}

namespace resources::types {

    class EntityScripts : public million::api::resources::Loader {
    public:
        EntityScripts(core::Engine*) {}
        virtual ~EntityScripts() {}

        bool cached (const std::string& filename, std::uint32_t*) final; // entity-script's are cached
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "entity-script"_hs;
    private:
        helpers::hashed_string_map<std::uint32_t, helpers::thread_safe_flat_map> m_cached_ids;
    };
     

    class SceneScripts : public million::api::resources::Loader {
    public:
        SceneScripts(core::Engine*) {}
        virtual ~SceneScripts() {}

        bool cached (const std::string& filename, std::uint32_t*) final { return false; } // No caching
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-script"_hs;
    };

    class SceneEntities : public million::api::resources::Loader {
    public:
        SceneEntities(core::Engine*);
        virtual ~SceneEntities() {}

        bool cached (const std::string& filename, std::uint32_t*) final { return false; } // No caching
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-entities"_hs;
    private:
        core::Engine* m_engine;
    };
}
