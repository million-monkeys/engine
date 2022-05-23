#pragma once

#include <monkeys.hpp>
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
        void loadScene (entt::hashed_string::hash_type scene, bool auto_swap);

        void update ();
        void swapScenes ();

        void processEvents ();

        entt::hashed_string::hash_type current () const { return m_current.scene; }
        million::resources::Handle handle () const { return m_current.scripts; }

    private:
        struct PendingScene {
            phmap::flat_hash_set<million::resources::Handle::Type> resources;
            bool auto_swap;
        };
        core::Engine& m_engine;
        million::events::Stream& m_stream;
        std::filesystem::path m_path;
        helpers::hashed_string_flat_map<std::string> m_scenes;
        helpers::hashed_string_flat_map<PendingScene> m_pending_scenes;
        struct Info {
            entt::hashed_string::hash_type scene = 0;
            million::resources::Handle scripts;
        };
        Info m_current;
        Info m_pending;
        
    };

} // world::
