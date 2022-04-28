
#include "engine.hpp"

entt::organizer& core::Engine::organizer(million::SystemStage type)
{
    return m_scheduler.organizer(type);
}

entt::registry& core::Engine::registry(million::Registry which)
{
    switch (which) {
    case million::Registry::Runtime:
        return m_runtime_registry;
    case million::Registry::Background:
        return m_background_registry;
    case million::Registry::Prototype:
        return m_prototype_registry;
    };
}

entt::entity core::Engine::findEntity (entt::hashed_string name) const
{
    auto it = m_named_entities.find(name);
    if (it != m_named_entities.end()) {
        return it->second.entity;
    }
    return entt::null;
}

const std::string& core::Engine::findEntityName (const components::core::Named& named) const
{
    auto it = m_named_entities.find(named.name);
    if (it != m_named_entities.end()) {
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

void core::Engine::copyRegistry (const entt::registry& from, entt::registry& to)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    from.each([&from,&to](const auto source_entity) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            auto destination_storage = to.storage(id);
            if(destination_storage != nullptr && source_storage.contains(source_entity)) {
                destination_storage->emplace(destination_entity, source_storage.get(source_entity));
            }
        }
    });

}

void core::Engine::onAddNamedEntity (entt::registry& registry, entt::entity entity)
{
    const auto& named = registry.get<components::core::Named>(entity);
    m_named_entities[named.name] = {entity, named.name.data()};
}

void core::Engine::onRemoveNamedEntity (entt::registry& registry, entt::entity entity)
{
    const auto& named = registry.get<components::core::Named>(entity);
    m_named_entities.erase(named.name);
}
