
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
        auto& scene_registry = m_engine.registry(which);
        auto& prototype_registry = m_engine.registry(million::Registry::Prototype);

        // Unload previous scene, if there is one
        if (m_current_scene != entt::hashed_string{}) {
            spdlog::info("[scenes] Unloading scene: {}", m_scenes[m_current_scene]);
            // m_engine.callModuleHook<CM::UNLOAD_SCENE>();
            // Destroy all entities that aren't marked as global
            scene_registry.each([&scene_registry](auto entity){
                if (! scene_registry.all_of<components::core::Global>(entity)) {
                    scene_registry.destroy(entity);
                }
            });
            // Destroy all prototype entities that aren't marked as global
            prototype_registry.each([&prototype_registry](auto entity){
                if (! prototype_registry.all_of<components::core::Global>(entity)) {
                    prototype_registry.destroy(entity);
                }
            });
        }
        spdlog::info("[scenes] Loading scene: {}", scene_name);
        const auto config = parser::parse_toml(filename);

        // Load prototype entities
        if (config.contains("prototypes")) {
            for (const auto& entity : config.at("prototypes").as_array()) {
                if (entity.contains("_name_")) {
                    const auto& name = entity.at("_name_").as_string().str;
                    SPDLOG_TRACE("[scenes] Creating new prototype entity: {}", name);
                    auto entity_id = prototype_registry.create();
                    prototype_registry.emplace<core::EntityPrototypeID>(entity_id, entt::hashed_string::value(name.c_str()));
                    for (const auto& [name_str, component]  : entity.as_table()) {
                        SPDLOG_TRACE("[scenes] Adding component to prototype entity {}: {}", name, name_str);
                        toml::value value = component;
                        m_engine.loadComponent(prototype_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                    }
                } else {
                    spdlog::warn("[scenes] Entity prototype without _name_!");
                }
            }
        }

        // Load entities to scene
        if (config.contains("entity")) {
            for (const auto& entity : config.at("entity").as_array()) {
                auto entity_id = scene_registry.create();
                SPDLOG_TRACE("[scenes] Creating new entity: {}", entt::to_integral(entity_id));
                for (const auto& [name_str, component]  : entity.as_table()) {
                    SPDLOG_TRACE("[scenes] Adding component to entity {}: {}", entt::to_integral(entity_id), name_str);
                    toml::value value = component;
                    m_engine.loadComponent(scene_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                }
            }
        }

        // Load scripts
        if (config.contains("script-file") && config.contains("event-map")) {
            [[maybe_unused]] auto handle = resources::load("scripted-events"_hs, filename, "scene-script"_hs);
        }

        m_current_scene = scene;
        // m_engine.callModuleHook<CM::LOAD_SCENE>(scene);
    } else {
        spdlog::error("[scenes] Could not load scene because it does not exist: {}", scene);
    }
}
