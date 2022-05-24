#pragma once

#include <monkeys.hpp>
#include "resources/resources.hpp"

namespace loaders {
    class SceneEntities : public million::api::resources::Loader {
    public:
        SceneEntities (world::Context* context) : m_context (context) {}
        virtual ~SceneEntities() {}

        bool cached (const std::string& filename, std::uint32_t*) final { return false; } // No caching
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-entities"_hs;

    private:
        world::Context* m_context;
    };
}