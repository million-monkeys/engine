
#include "engine.hpp"

void mergeEntityInternal (entt::registry& source_registry, entt::registry& destination_registry, entt::entity source_entity, entt::entity destination_entity, bool overwrite_components)
{
    for(auto [id, source_storage]: source_registry.storage()) {
        auto it = destination_registry.storage(id);
        if (it != destination_registry.storage().end() && source_storage.contains(source_entity)) {
            auto& destination_storage = it->second;
            if (! destination_storage.contains(destination_entity)) {
                destination_storage.emplace(destination_entity, source_storage.get(source_entity));
            } else if (overwrite_components) {
                destination_storage.erase(destination_entity);
                destination_storage.emplace(destination_entity, source_storage.get(source_entity));
            }
        }
    }
}

entt::entity core::Engine::loadEntity (million::Registry which, entt::hashed_string prototype_id)
{
    EASY_FUNCTION(profiler::colors::Yellow100);
    auto it = m_prototype_entities.find(prototype_id);
    if (it != m_prototype_entities.end()) {
        auto new_entity = m_runtime_registry.create();
        mergeEntityInternal(m_prototype_registry, registry(which), it->second, new_entity, true);
        return new_entity;
    } else {
        spdlog::warn("Could not create entity. Prototype does not exist: \"{}\"", prototype_id.data());
    }
    return entt::null;
}

void core::Engine::mergeEntity (million::Registry which, entt::entity entity, entt::hashed_string prototype_id, bool overwrite_components)
{
    EASY_FUNCTION(profiler::colors::Yellow100);
    auto it = m_prototype_entities.find(prototype_id);
    if (it != m_prototype_entities.end()) {
        mergeEntityInternal(m_prototype_registry, registry(which), it->second, entity, overwrite_components);
    }
}

void core::Engine::onAddPrototypeEntity (entt::registry& registry, entt::entity entity)
{
    const auto& prototype_id = registry.get<core::EntityPrototypeID>(entity);
    auto it = m_prototype_entities.find(prototype_id.id);
    if (it != m_prototype_entities.end()) {
        // Already exists, destroy the old one before replacing it with the new one.
        m_prototype_registry.destroy(it->second);
    }
    m_prototype_entities[prototype_id.id] = entity;
}

void core::Engine::onRemovePrototypeEntity (entt::registry& registry, entt::entity entity)
{
    const auto& prototype_id = registry.get<core::EntityPrototypeID>(entity);
    m_prototype_entities.erase(prototype_id.id);
}
