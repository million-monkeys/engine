#pragma once

#include <game.hpp>
#include <filesystem>

namespace core
{
    class Engine;
} // core::

namespace world {

    class SceneManager {
    public:
        SceneManager (core::Engine&);
        ~SceneManager ();

        void loadSceneList (const std::string& path);
        void loadScene (million::Registry which, entt::hashed_string::hash_type scene, bool auto_swap);

        void update ();
        void swapScenes ();

        entt::hashed_string::hash_type current () const { return m_current_scene; }

    private:
        struct PendingScene {
            phmap::flat_hash_set<million::resources::Handle::Type> resources;
            bool auto_swap;
        };
        core::Engine& m_engine;
        std::filesystem::path m_path;
        helpers::hashed_string_flat_map<std::string> m_scenes;
        helpers::hashed_string_flat_map<PendingScene> m_pending_scenes;
        entt::hashed_string::hash_type m_current_scene;
    };

} // world::
