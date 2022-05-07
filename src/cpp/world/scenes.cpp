
#include "scenes.hpp"
#include "utils/parser.hpp"
#include <core/engine.hpp>
#include <physfs.hpp>

#include "resources/resources.hpp"
#include "scripting/scripting.hpp"

world::SceneManager::SceneManager (core::Engine& engine) :
    m_engine(engine),
    m_stream(engine.createStream("scenes"_hs))
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
            SPDLOG_DEBUG("[scenes] Adding scene: {} ({:x})", scene, scene_id);
            m_scenes[scene_id] = scene;
        }
    }
}

void world::SceneManager::loadScene (entt::hashed_string::hash_type scene, bool auto_swap)
{
    EASY_FUNCTION(profiler::colors::RichYellow);

    auto it = m_scenes.find(scene);
    if (it != m_scenes.end()) {        
        const auto scene_name = it->second;
        const auto filename = (m_path / scene_name).replace_extension("toml").string();

        spdlog::info("[scenes] Loading scene: {}", scene_name);
        const auto config = parser::parse_toml(filename);
        PendingScene& pending = m_pending_scenes[scene];
        pending.auto_swap = auto_swap;

        // Load entities and prototypes
        if (config.contains("entity")) {
            auto handle = resources::load("scene-entities"_hs, filename, scene);
            pending.resources.insert(handle.handle);
        }

        // Preload resources
        if (config.contains("resources")) {
            for (const auto& [name, resource] : config.at("resources").as_table()) {
                if (resource.is_table() && resource.contains("type") && resource.contains("file")) {
                    auto& type = resource.at("type");
                    auto& file = resource.at("file");
                    if (! type.is_string()) {
                        spdlog::error("[scenes] In scene file '{}', resource '{}' has invalid type field", scene_name, name);
                        continue;
                    }
                    if (! file.is_string()) {
                        spdlog::error("[scenes] In scene file '{}', resource '{}' has invalid file field", scene_name, name);
                        continue;
                    }
                    // Queue resource for loading
                    auto handle = resources::load(entt::hashed_string{type.as_string().str.c_str()}, file.as_string(), scene);
                    // Bind resource to name
                    m_engine.bindResourceToName(handle, entt::hashed_string::value(name.c_str()));
                    // Add resoruce to pending for scene
                    pending.resources.insert(handle.handle);
                }
            }
        }

        // Load scripts
        if (config.contains("script-file") && config.contains("event-map")) {
            auto handle = resources::load("scene-script"_hs, filename, scene);
            pending.resources.insert(handle.handle);
        }

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
                    case events::resources::Loaded::ID:
                    {
                        auto& loaded = m_engine.eventData<events::resources::Loaded>(ev);
                        auto it = m_pending_scenes.find(loaded.name);
                        if (it != m_pending_scenes.end()) {
                            PendingScene& pending = it->second;
                            pending.resources.erase(loaded.handle.handle);
                            if (loaded.type == "scene-script"_hs) {
                                m_pending.scripts = loaded.handle;
                            }
                            if (pending.resources.empty()) {
                                // Scene fully loaded
                                m_pending.scene = loaded.name;
                                m_stream.emit<events::scenes::Loaded>([&loaded](auto& scene){
                                    scene.id = loaded.name;
                                });
                                if (pending.auto_swap) {
                                    swapScenes();
                                }
                                m_pending_scenes.erase(it);
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

// Swap foreground and background scenes, and clear the (new) background scene
void world::SceneManager::swapScenes ()
{
    // Call UNLOAD_SCENE hooks on old scene
    // m_engine.callModuleHook<core::CM::UNLOAD_SCENE>(m_current_scene, m_scenes[m_current_scene]);

    // Set current scene and scripts
    m_current.scene = m_pending.scene;
    m_current.scripts = m_pending.scripts;

    // Invalidate pending
    m_pending.scene = 0;
    m_pending.scripts = million::resources::Handle::invalid();

    // Swap newly loaded scene into foreground
    m_engine.m_registries.swap();
    // Copy entities marked as "global" from background to foreground
    m_engine.m_registries.copyGlobals();
    // Clear the background registry
    m_engine.m_registries.background().clear();

    // Call LOAD_SCENE hooks on new scene
    // TODO: Create a scene API object to pass in? What can it do?
    // m_engine.callModuleHook<core::CM::LOAD_SCENE>(m_current_scene, m_scenes[m_current_scene]);

    m_stream.emit<events::scenes::Activated>([this](auto& scene){
        scene.id = m_current.scene;
    });
}

void world::SceneManager::processEvents ()
{
    if (m_current.scripts.valid()) {
        scripting::processSceneEvents(m_current.scripts);
    }
}
