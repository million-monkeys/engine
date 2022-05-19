
#include "scenes.hpp"
#include "context.hpp"

void mergeEntityInternal (entt::registry& source_registry, entt::registry& destination_registry, entt::entity source_entity, entt::entity destination_entity, bool overwrite_components)
{
    EASY_FUNCTION(profiler::colors::Yellow200);
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

entt::entity scenes::loadEntity (scenes::Context* context, entt::hashed_string prototype_id)
{
    EASY_FUNCTION(profiler::colors::Yellow100);
    const auto& prototype_names = context->m_registries.foreground().prototype_names;
    auto it = prototype_names.find(prototype_id);
    if (it != prototype_names.end()) {
        auto& foreground = context->m_registries.foreground();
        auto& prototypes = foreground.prototypes;
        auto new_entity = prototypes.create();
        mergeEntityInternal(prototypes, foreground.runtime, it->second, new_entity, true);
        return new_entity;
    } else {
        spdlog::warn("Could not create entity. Prototype does not exist: \"{}\"", prototype_id.data());
    }
    return entt::null;
}

void scenes::mergeEntity (scenes::Context* context, entt::entity entity, entt::hashed_string prototype_id, bool overwrite_components)
{
    EASY_FUNCTION(profiler::colors::Yellow100);
    const auto& prototype_names = context->m_registries.foreground().prototype_names;
    auto it = prototype_names.find(prototype_id);
    if (it != prototype_names.end()) {
        auto& foreground = context->m_registries.foreground();
        mergeEntityInternal(foreground.prototypes, foreground.runtime, it->second, entity, overwrite_components);
    }
}
