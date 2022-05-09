
#include "resources/builtintypes.hpp"
#include "scripting/scripting.hpp"
#include "utils/parser.hpp"

#include "core/engine.hpp"

resources::types::SceneEntities::SceneEntities(core::Engine* engine)
    : m_engine(engine)
{
}

bool resources::types::SceneEntities::load (million::resources::Handle handle, const std::string& filename)
{
    EASY_FUNCTION(profiler::colors::Teal300);
    try {
        auto config = parser::parse_toml(filename);

        auto& registries = m_engine->backgroundRegistries();

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
                        if (name_str != "_name_") {
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
                    toml::value value = component;
                    m_engine->loadComponent(scene_registry, entt::hashed_string{name_str.c_str()}, entity_id, reinterpret_cast<const void*>(&value));
                }
            }
        }

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:scene-entities] '{}' file not found", filename);
        return false;
    }

    return true;
}

void resources::types::SceneEntities::unload (million::resources::Handle handle)
{

}
