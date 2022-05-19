#include "scenes.hpp"
#include "context.hpp"

const std::string g_empty_string = {};

entt::entity scenes::findEntity (scenes::Context* context, entt::hashed_string name)
{
    EASY_FUNCTION(scenes::COLOR(2));
    const auto& entities = context->m_registries.foreground().entity_names;
    auto it = entities.find(name);
    if (it != entities.end()) {
        return it->second.entity;
    }
    return entt::null;
}

const std::string& scenes::findEntityName (scenes::Context* context, const components::core::Named& named)
{
    EASY_FUNCTION(scenes::COLOR(2));
    const auto& entities = context->m_registries.foreground().entity_names;
    auto it = entities.find(named.name);
    if (it != entities.end()) {
        return it->second.name;
    } else {
        spdlog::warn("No name for {}", named.name.data());
    }
    return g_empty_string;
}

bool scenes::isInGroup (scenes::Context* context, entt::entity entity, entt::hashed_string::hash_type group_name)
{
    const auto& registry = context->m_registries.foreground().runtime;
    const auto& storage = registry.storage<EntityGroup>(group_name);
    return storage.contains(entity);
}
