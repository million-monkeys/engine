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
        void loadScene (million::Registry which, entt::hashed_string::hash_type scene);

        entt::hashed_string::hash_type current () const { return m_current_scene; }

    private:
        core::Engine& m_engine;
        std::filesystem::path m_path;
        phmap::flat_hash_map<entt::hashed_string::hash_type, std::string, helpers::Identity> m_scenes;
        entt::hashed_string::hash_type m_current_scene;
    };

} // world::
