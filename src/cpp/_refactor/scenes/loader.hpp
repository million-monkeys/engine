#pragma once

#include "resources/resources.hpp"

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
