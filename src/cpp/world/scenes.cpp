
#include "scenes.hpp"
#include "utils/parser.hpp"
#include <core/engine.hpp>
#include <physfs.hpp>

#include "resources/resources.hpp"

world::SceneManager::SceneManager (core::Engine& engine) :
    m_engine(engine),
    m_current_scene(entt::hashed_string{})
{

}

world::SceneManager::~SceneManager ()
{

}

void world::SceneManager::loadSceneList (const std::string& path)
{
    // Clear previous scenes, if any
    m_scenes.clear();

    m_path = path;
    for (const auto& filename : physfs::enumerateFiles(path)) {
        auto file = std::filesystem::path(filename);
        if (file.extension() == ".toml") {
            auto scene = file.replace_extension("").string();
            auto scene_id = entt::hashed_string::value(scene.c_str());
            SPDLOG_DEBUG("[scenes] Adding scene: {} ({})", scene, scene_id);
            m_scenes[scene_id] = scene;
        }
    }
}

void world::SceneManager::loadScene (million::Registry which, entt::hashed_string::hash_type scene)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    // using CM = gou::api::Module::CallbackMasks;

    auto it = m_scenes.find(scene);
    if (it != m_scenes.end()) {        
        const auto scene_name = it->second;
        const auto filename = (m_path / scene_name).replace_extension("toml").string();
        // auto& scene_registry = m_engine.registry(which);
        // auto& prototype_registry = m_engine.registry(million::Registry::Prototype);

        // // Unload previous scene, if there is one
        // if (m_current_scene != entt::hashed_string{}) {
        //     spdlog::info("[scenes] Unloading scene: {}", m_scenes[m_current_scene]);
        //     // m_engine.callModuleHook<CM::UNLOAD_SCENE>();
        //     // Destroy all entities that aren't marked as global
        //     scene_registry.each([&scene_registry](auto entity){
        //         if (! scene_registry.all_of<components::core::Global>(entity)) {
        //             scene_registry.destroy(entity);
        //         }
        //     });
        //     // Destroy all prototype entities that aren't marked as global
        //     prototype_registry.each([&prototype_registry](auto entity){
        //         if (! prototype_registry.all_of<components::core::Global>(entity)) {
        //             prototype_registry.destroy(entity);
        //         }
        //     });
        // }
        spdlog::info("[scenes] Loading scene: {}", scene_name);
        const auto config = parser::parse_toml(filename);

        if (config.contains("entity")) {
            auto handle = resources::load("scene-entities"_hs, filename, scene);
            m_pending_scenes[scene].insert(handle.handle);
        }

        // Load scripts
        if (config.contains("script-file") && config.contains("event-map")) {
            auto handle = resources::load("scene-script"_hs, filename, scene);
            m_pending_scenes[scene].insert(handle.handle);
        }

        m_current_scene = scene;
        // m_engine.callModuleHook<CM::LOAD_SCENE>(scene);
    } else {
        spdlog::error("[scenes] Could not load scene because it does not exist: {}", scene);
    }
}

void world::SceneManager::update ()
{
    if (! m_pending_scenes.empty()) {
        auto iter = m_engine.events("resources"_hs);
        if (iter.size() > 0) {
            EASY_BLOCK("SceneManager handling resource events", profiler::colors::Amber200);
            for (const auto& ev : iter) {   
                switch (ev.type) {
                    case "loaded"_hs:
                    {
                        auto& loaded = m_engine.eventData<events::engine::ResourceLoaded>(ev);
                        if (loaded.type == "scene-entities"_hs || loaded.type == "scene-script"_hs) {
                            auto it = m_pending_scenes.find(loaded.name);
                            if (it != m_pending_scenes.end()) {
                                it->second.erase(loaded.handle.handle);
                                if (it->second.empty()) {
                                    // Scene fully loaded
                                    spdlog::error("Scene fully loaded (name:{}, handle:{}", loaded.name, loaded.handle.id());
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                };
            }
        }
    }
}