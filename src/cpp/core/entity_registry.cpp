
#include "engine.hpp"

core::RegistryPair::RegistryPair()
{
    // Manage Named entities
    runtime.on_construct<components::core::Named>().connect<&core::RegistryPair::onAddNamedEntity>(this);
    runtime.on_destroy<components::core::Named>().connect<&core::RegistryPair::onRemoveNamedEntity>(this);
    // Manage prototype entities
    prototypes.on_construct<core::EntityPrototypeID>().connect<&core::RegistryPair::onAddPrototypeEntity>(this);
    prototypes.on_destroy<core::EntityPrototypeID>().connect<&core::RegistryPair::onRemovePrototypeEntity>(this);
}

core::RegistryPair::~RegistryPair()
{

}

void core::RegistryPair::onAddNamedEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Green500);
    const auto& named = registry.get<components::core::Named>(entity);
    entity_names[named.name] = {entity, named.name.data()};
}

void core::RegistryPair::onRemoveNamedEntity (entt::registry& registry, entt::entity entity)
{
    EASY_FUNCTION(profiler::colors::Green500);
    const auto& named = registry.get<components::core::Named>(entity);
    entity_names.erase(named.name);
}

void core::RegistryPair::clear ()
{
    EASY_FUNCTION(profiler::colors::Green800);
    runtime = {};
    prototypes = {};
    entity_names.clear();
    prototype_names.clear();
}

entt::organizer& core::Engine::organizer(million::SystemStage type)
{
    return m_scheduler.organizer(type);
}

entt::registry& core::Engine::registry(million::Registry which)
{
    switch (which) {
    case million::Registry::Runtime:
        return m_registries.foreground().runtime;
    case million::Registry::Background:
        return m_registries.background().runtime;
    case million::Registry::Prototype:
        return m_registries.foreground().prototypes;
    };
}

entt::entity core::Engine::findEntity (entt::hashed_string name) const
{
    EASY_FUNCTION(profiler::colors::Green200);
    const auto& entities = m_registries.foreground().entity_names;
    auto it = entities.find(name);
    if (it != entities.end()) {
        return it->second.entity;
    }
    return entt::null;
}

const std::string& core::Engine::findEntityName (const components::core::Named& named) const
{
    EASY_FUNCTION(profiler::colors::Green200);
    const auto& entities = m_registries.foreground().entity_names;
    auto it = entities.find(named.name);
    if (it != entities.end()) {
        return it->second.name;
    } else {
        spdlog::warn("No name for {}", named.name.data());
    }
    return m_empty_string;
}

void core::Engine::loadComponent (entt::registry& registry, entt::hashed_string component, entt::entity entity, const void* table)
{
    EASY_FUNCTION(profiler::colors::Green100);
    auto it = m_component_loaders.find(component);
    if (it != m_component_loaders.end()) {
        const auto& loader = it->second;
        loader(this, registry, table, entity);
    } else {
        spdlog::warn("Tried to load non-existent component: {}", component.data());
    }
}

void core::Engine::Registries::copyRegistry (const entt::registry& from, entt::registry& to)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    from.each([&from,&to](const auto source_entity) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            if(source_storage.contains(source_entity)) {
                auto it = to.storage(id);
                if (it != to.storage().end()) {
                    it->second.emplace(destination_entity, source_storage.get(source_entity));
                }
            }
        }
    });
}

void core::Engine::Registries::copyGlobals ()
{
    // Entity names will be automatically copied by on_construct
    EASY_FUNCTION(profiler::colors::RichYellow);
    auto& from = background().runtime;
    auto& to = foreground().runtime;
    for (const auto&& [source_entity] : from.storage<components::core::Global>().each()) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            if(source_storage.contains(source_entity)) {
                auto it = to.storage(id);
                if (it != to.storage().end()) {
                    it->second.emplace(destination_entity, source_storage.get(source_entity));
                }
            }
        }
    }
}
