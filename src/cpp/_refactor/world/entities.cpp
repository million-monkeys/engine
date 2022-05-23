#include "world.hpp"
#include "context.hpp"

const std::string g_empty_string = {};

entt::entity world::findEntity (world::Context* context, entt::hashed_string name)
{
    EASY_FUNCTION(world::COLOR(2));
    const auto& entities = context->m_registries.foreground().entity_names;
    auto it = entities.find(name);
    if (it != entities.end()) {
        return it->second.entity;
    }
    return entt::null;
}

const std::string& world::findEntityName (world::Context* context, const components::core::Named& named)
{
    EASY_FUNCTION(world::COLOR(2));
    const auto& entities = context->m_registries.foreground().entity_names;
    auto it = entities.find(named.name);
    if (it != entities.end()) {
        return it->second.name;
    } else {
        spdlog::warn("No name for {}", named.name.data());
    }
    return g_empty_string;
}

bool world::isInGroup (world::Context* context, entt::entity entity, entt::hashed_string::hash_type group_name)
{
    const auto& registry = context->m_registries.foreground().runtime;
    const auto& storage = registry.storage<EntityGroup>(group_name);
    return storage.contains(entity);
}

void world::setEntityCategories (world::Context* context, const std::vector<entt::hashed_string::hash_type>& entity_categories)
{
    if (entity_categories.size() > 16) {
        spdlog::error("Cannot have more than 16 entity categories, but {} were declared!", entity_categories.size());
    }
    std::uint16_t bit_index = 0;
    for (const auto& category : entity_categories) {
        context->m_category_bitfields.emplace(category, 1 << bit_index++);
    }
}

std::uint16_t world::categoryBitflag (world::Context* context, entt::hashed_string::hash_type category_name)
{
    auto it = context->m_category_bitfields.find(category_name);
    if (it != context->m_category_bitfields.end()) {
        return it->second;
    }
    return 0;
}
