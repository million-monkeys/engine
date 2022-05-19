#include "scenes.hpp"
#include "loader.hpp"
#include "context.hpp"

#include "scripting/scripting.hpp"
#include "utils/parser.hpp"

#include "core/engine.hpp"

#include <physfs.hpp>

void scenes::loadSceneList (scenes::Context* context, const std::string& path)
{
    EASY_FUNCTION(profiler::colors::Pink100);
    // Clear previous scenes, if any
    context->m_scenes.clear();

    context->m_path = path;
    for (const auto& filename : physfs::enumerateFiles(path)) {
        auto file = std::filesystem::path(filename);
        if (file.extension() == ".toml") {
            auto scene = file.replace_extension("").string();
            auto scene_id = entt::hashed_string::value(scene.c_str());
            SPDLOG_DEBUG("[scenes] Adding scene: {} ({:x})", scene, scene_id);
            context->m_scenes[scene_id] = scene;
        }
    }
}

void scenes::loadScene (scenes::Context* context, entt::hashed_string::hash_type scene, bool auto_swap)
{
    EASY_FUNCTION(profiler::colors::RichYellow);

    auto it = context->m_scenes.find(scene);
    if (it != context->m_scenes.end()) {        
        const auto scene_name = it->second;
        const auto filename = (context->m_path / scene_name).replace_extension("toml").string();

        spdlog::info("[scenes] Loading scene: {}", scene_name);
        const auto config = parser::parse_toml(filename);
        PendingScene& pending = context->m_pending_scenes[scene];
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
                    // TODO: m_engine.bindResourceToName(handle, entt::hashed_string::value(name.c_str()));
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


SceneEntities::SceneEntities(core::Engine* engine)
    : m_engine(engine)
{
}

void addGroups (entt::registry& registry, const TomlValue& groups, entt::entity entity)
{
    if (groups.is_array()) {
        for (auto& group_name : groups.as_array()) {
            if (group_name.is_string()) {
                auto& storage = registry.storage<core::EntityGroup>(entt::hashed_string::value(group_name.as_string().str.c_str()));
                storage.emplace(entity);
            } else {
                spdlog::warn("[scenes] Group name must be string");
            }
        }
    }
}

bool SceneEntities::load (million::resources::Handle handle, const std::string& filename)
{
    EASY_FUNCTION(profiler::colors::Teal300);
    try {
        auto config = parser::parse_toml(filename);

        auto& registries = m_engine->registries(core::RegistryPair::Registries::Background);

        // Load prototype entities
        if (config.contains("prototypes")) {
            auto& prototype_registry = registries.prototypes;
            for (const auto& entity : config.at("prototypes").as_array()) {
                if (entity.contains("_name_")) {
                    const auto& name = entity.at("_name_").as_string().str;
                    SPDLOG_TRACE("[scenes] Creating new prototype entity: {}", name);
                    auto entity_id = prototype_registry.create();
                    prototype_registry.emplace<core::EntityPrototypeID>(entity_id, entt::hashed_string::value(name.c_str()));
                    for (const auto& [name_str, component]  : entity.as_table()) {
                        if (name_str == "_groups_") {
                            // Add entity to groups
                            addGroups(prototype_registry, component, entity_id);
                        } else if (name_str == "_category_") {
                            if (! component.is_string()) {
                                spdlog::warn("[scenes] Error loading entity: _category_ field must be a string");
                                continue;
                            }
                            const auto& name = component.as_string().str;
                            std::uint16_t bitfield = m_engine->categoryBitflag(entt::hashed_string::value(name.c_str()));
                            prototype_registry.emplace<components::core::Category>(entity_id, bitfield);
                        } else if (name_str != "_name_") {
                            SPDLOG_TRACE("[scenes] Adding component to prototype entity {}: {}", name, name_str);
                            toml::value value = component;
                            m_engine->loadComponent(prototype_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                        }
                    }
                } else {
                    spdlog::warn("[scenes] Entity prototype without _name_!");
                }
            }
        }

        // Load entities to scene
        if (config.contains("entity")) {
            auto& scene_registry = registries.runtime;
            for (const auto& entity : config.at("entity").as_array()) {
                auto entity_id = scene_registry.create();
                SPDLOG_TRACE("[scenes] Creating new entity: {}", entt::to_integral(entity_id));
                for (const auto& [name_str, component]  : entity.as_table()) {
                    SPDLOG_TRACE("[scenes] Adding component to entity {}: {}", entt::to_integral(entity_id), name_str);
                    if (name_str == "_groups_") {
                        // Add entity to groups
                        addGroups(scene_registry, component, entity_id);
                    } else if (name_str == "_name_") {
                        if (! component.is_string()) {
                            spdlog::warn("[scenes] Error loading entity: _name_ field must be a string");
                            continue;
                        }
                        const auto& name = component.as_string().str;
                        scene_registry.emplace<components::core::Named>(entity_id, entt::hashed_string{name.c_str()});
                    } else if (name_str == "_category_") {
                        if (! component.is_string()) {
                            spdlog::warn("[scenes] Error loading entity: _category_ field must be a string");
                            continue;
                        }
                        const auto& name = component.as_string().str;
                        std::uint16_t bitfield = m_engine->categoryBitflag(entt::hashed_string::value(name.c_str()));
                        scene_registry.emplace<components::core::Category>(entity_id, bitfield);
                    } else {
                        toml::value value = component;
                        m_engine->loadComponent(scene_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                    }
                    
                }
            }
        }

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:scene-entities] '{}' file not found", filename);
        return false;
    }

    return true;
}

void SceneEntities::unload (million::resources::Handle handle)
{

}
