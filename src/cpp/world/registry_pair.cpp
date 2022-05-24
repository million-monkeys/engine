
#include "registry_pair.hpp"

#include "core/components.hpp"

RegistryPair::RegistryPair()
{
    // Manage Named entities
    runtime.on_construct<components::core::Named>().connect<&RegistryPair::onAddNamedEntity>(this);
    runtime.on_destroy<components::core::Named>().connect<&RegistryPair::onRemoveNamedEntity>(this);
    // Manage prototype entities
    prototypes.on_construct<core::EntityPrototypeID>().connect<&RegistryPair::onAddPrototypeEntity>(this);
    prototypes.on_destroy<core::EntityPrototypeID>().connect<&RegistryPair::onRemovePrototypeEntity>(this);
}

RegistryPair::~RegistryPair()
{

}

void RegistryPair::onAddNamedEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Green500);
    const auto& named = registry.get<components::core::Named>(entity);
    entity_names[named.name] = {entity, named.name.data()};
}

void RegistryPair::onRemoveNamedEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Green500);
    const auto& named = registry.get<components::core::Named>(entity);
    entity_names.erase(named.name);
}

void RegistryPair::clear ()
{
    EASY_BLOCK("RegistryPair::clear", profiler::colors::Green800);
    runtime.clear();
    prototypes.clear();
    entity_names.clear();
    prototype_names.clear();
}

void RegistryPair::onAddPrototypeEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Yellow500);
    const auto& prototype_id = registry.get<core::EntityPrototypeID>(entity);
    auto it = prototype_names.find(prototype_id.id);
    if (it != prototype_names.end()) {
        // Already exists, destroy the old one before replacing it with the new one.
        registry.destroy(it->second);
    }
    prototype_names[prototype_id.id] = entity;
}

void RegistryPair::onRemovePrototypeEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Yellow500);
    const auto& prototype_id = registry.get<core::EntityPrototypeID>(entity);
    prototype_names.erase(prototype_id.id);
}
