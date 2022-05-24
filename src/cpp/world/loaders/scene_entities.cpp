#include "scene_entities.hpp"
#include "../world.hpp"
#include "../context.hpp"

#include "core/components.hpp"
#include "scripting/scripting.hpp"
#include "utils/parser.hpp"

void addGroups (entt::registry& registry, const TomlValue& groups, entt::entity entity)
{
    if (groups.is_array()) {
        for (auto& group_name : groups.as_array()) {
            if (group_name.is_string()) {
                auto& storage = registry.storage<core::EntityGroup>(entt::hashed_string::value(group_name.as_string().str.c_str()));
                storage.emplace(entity);
            } else {
                spdlog::warn("[scene] Group name must be string");
            }
        }
    }
}

bool loaders::SceneEntities::load (million::resources::Handle handle, const std::string& filename)
{
    EASY_BLOCK("SceneEntities::load", world::COLOR(3));
    try {
        auto config = parser::parse_toml(filename);

        auto& registries = m_context->m_registries.background();

        // Load prototype entities
        if (config.contains("prototypes")) {
            auto& prototype_registry = registries.prototypes;
            for (const auto& entity : config.at("prototypes").as_array()) {
                if (entity.contains("_name_")) {
                    const auto& name = entity.at("_name_").as_string().str;
                    SPDLOG_TRACE("[scene] Creating new prototype entity: {}", name);
                    auto entity_id = prototype_registry.create();
                    prototype_registry.emplace<core::EntityPrototypeID>(entity_id, entt::hashed_string::value(name.c_str()));
                    for (const auto& [name_str, component]  : entity.as_table()) {
                        if (name_str == "_groups_") {
                            // Add entity to groups
                            addGroups(prototype_registry, component, entity_id);
                        } else if (name_str == "_category_") {
                            if (! component.is_string()) {
                                spdlog::warn("[scene] Error loading entity: _category_ field must be a string");
                                continue;
                            }
                            const auto& name = component.as_string().str;
                            std::uint16_t bitfield = world::categoryBitflag(m_context, entt::hashed_string::value(name.c_str()));
                            prototype_registry.emplace<components::core::Category>(entity_id, bitfield);
                        } else if (name_str != "_name_") {
                            SPDLOG_TRACE("[scene] Adding component to prototype entity {}: {}", name, name_str);
                            toml::value value = component;
                            loadComponent(m_context, prototype_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                        }
                    }
                } else {
                    spdlog::warn("[scene] Entity prototype without _name_!");
                }
            }
        }

        // Load entities to scene
        if (config.contains("entity")) {
            auto& scene_registry = registries.runtime;
            for (const auto& entity : config.at("entity").as_array()) {
                auto entity_id = scene_registry.create();
                SPDLOG_TRACE("[scene] Creating new entity: {}", entt::to_integral(entity_id));
                for (const auto& [name_str, component]  : entity.as_table()) {
                    if (name_str == "_groups_") {
                        // Add entity to groups
                        SPDLOG_TRACE("[scene] Setting entity {} groups", entt::to_integral(entity_id));
                        addGroups(scene_registry, component, entity_id);
                    } else if (name_str == "_name_") {
                        if (! component.is_string()) {
                            spdlog::warn("[scene] Error loading entity: _name_ field must be a string");
                            continue;
                        }
                        const auto& name = component.as_string().str;
                        SPDLOG_TRACE("[scene] Setting entity {} name to: {}", entt::to_integral(entity_id), name);
                        scene_registry.emplace<components::core::Named>(entity_id, entt::hashed_string{name.c_str()});
                    } else if (name_str == "_category_") {
                        if (! component.is_string()) {
                            spdlog::warn("[scene] Error loading entity: _category_ field must be a string");
                            continue;
                        }
                        const auto& name = component.as_string().str;
                        SPDLOG_TRACE("[scene] Setting entity {} category to: {}", entt::to_integral(entity_id), name);
                        std::uint16_t bitfield = world::categoryBitflag(m_context, entt::hashed_string::value(name.c_str()));
                        scene_registry.emplace<components::core::Category>(entity_id, bitfield);
                    } else {
                        SPDLOG_TRACE("[scene] Adding component to entity {}: {}", entt::to_integral(entity_id), name_str);
                        toml::value value = component;
                        loadComponent(m_context, scene_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
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

void loaders::SceneEntities::unload (million::resources::Handle handle)
{
    EASY_BLOCK("SceneEntities::unload", world::COLOR(3));
}
