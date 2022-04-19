
#include "scenes.hpp"
#include "utils/parser.hpp"
#include <core/engine.hpp>
#include <physfs.hpp>

world::SceneManager::SceneManager (core::Engine& engine) :
    m_engine(engine)
{

}

world::SceneManager::~SceneManager ()
{

}

void world::SceneManager::loadSceneList (const std::string& filename)
{
    // Clear previous scenes, if any
    m_scenes.clear();

    // Load new scenes
    const auto config = parser::parse_toml(filename);
    if (config.contains("scenes")) {
        const auto& scenes = config.at("scenes");
        for (const auto& [name, path]  : scenes.as_table()) {
            auto filename = path.as_string();
            if (physfs::exists(filename)) {
                m_scenes[entt::hashed_string{name.c_str()}] = filename;
            } else {
                spdlog::warn("Scene \"{}\" file does not exist: {}", name, filename);
            }
            
        }
    }
}

void world::SceneManager::loadScene (monkeys::Registry which, entt::hashed_string scene)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    // using CM = gou::api::Module::CallbackMasks;

    auto it = m_scenes.find(scene);
    if (it != m_scenes.end()) {
        auto& scene_registry = m_engine.registry(which);
        auto& prototype_registry = m_engine.registry(monkeys::Registry::Prototype);

        // Unload previous scene, if there is one
        if (m_current_scene != entt::hashed_string{}) {
            spdlog::info("[SceneManager] Unloading scene: {}", m_current_scene.data());
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
        spdlog::info("[SceneManager] Loading scene: {}", scene.data());
        const auto config = parser::parse_toml(it->second);

        // Load prototype entities
        if (config.contains("prototypes")) {
            for (const auto& entity : config.at("prototypes").as_array()) {
                if (entity.contains("_name_")) {
                    const auto& name = entity.at("_name_").as_string().str;
                    SPDLOG_TRACE("[SceneManager] Creating new prototype entity: {}", name);
                    auto entity_id = prototype_registry.create();
                    prototype_registry.emplace<core::EntityPrototypeID>(entity_id, entt::hashed_string::value(name.c_str()));
                    for (const auto& [name_str, component]  : entity.as_table()) {
                        SPDLOG_TRACE("[SceneManager] Adding component to prototype entity {}: {}", name, name_str);
                        toml::value value = component;
                        m_engine.loadComponent(prototype_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                    }
                } else {
                    spdlog::warn("[SceneManager] Entity prototype without _name_!");
                }
            }
        }
        // Load entities to scene
        if (config.contains("entity")) {
            for (const auto& entity : config.at("entity").as_array()) {
                auto entity_id = scene_registry.create();
                SPDLOG_TRACE("[SceneManager] Creating new entity: {}", entt::to_integral(entity_id));
                for (const auto& [name_str, component]  : entity.as_table()) {
                    SPDLOG_TRACE("[SceneManager] Adding component to entity {}: {}", entt::to_integral(entity_id), name_str);
                    toml::value value = component;
                    m_engine.loadComponent(scene_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                }
            }
        }

        m_current_scene = scene;
        // m_engine.callModuleHook<CM::LOAD_SCENE>(scene);
    } else {
        spdlog::error("[SceneManager] Could not load scene because it does not exist: {}", scene.data());
    }
}
