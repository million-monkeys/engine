#include "scripting.hpp"
#include "context.hpp"
#include "_refactor/world/world.hpp"

#include "core/engine.hpp"

extern "C" std::uint32_t null_entity_value ()
{
    return magic_enum::enum_integer(entt::entity{entt::null});
}

extern "C" std::uint32_t entity_create (scripting::Context* context)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    return magic_enum::enum_integer(registry.create());
}

extern "C" std::uint32_t entity_create_from_prototype (scripting::Context* context, const char* prototype)
{
    EASY_FUNCTION(scripting::COLOR(3));
    return magic_enum::enum_integer(world::loadEntity(context->m_world_ctx, entt::hashed_string{prototype}));
}

extern "C" void entity_destroy (scripting::Context* context, std::uint32_t entity)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    registry.destroy(static_cast<entt::entity>(entity));
}

extern "C" std::uint32_t entity_lookup_by_name (scripting::Context* context, const char* name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    return magic_enum::enum_integer(world::findEntity(context->m_world_ctx, entt::hashed_string{name}));
}

extern "C" bool entity_has_component (scripting::Context* context, std::uint32_t entity, const char* component_name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto it = context->m_component_types.find(entt::hashed_string::value(component_name));
    if (it != context->m_component_types.end()) {
        auto& storage = registry.storage(it->second)->second; // `it` would not be valid if this storage doesn't exist
        entt::entity e = static_cast<entt::entity>(entity);
        return storage.contains(e);
    } else {
        // Invalid component name
        spdlog::error("[script] component_get_for_entity invalid component name: {}", component_name);
    }
    return false;
}

extern "C" void entity_set_group (scripting::Context* context, uint32_t entity, const char* group_name, bool in_group)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto& storage = registry.storage<core::EntityGroup>(entt::hashed_string::value(group_name));
    entt::entity e = static_cast<entt::entity>(entity);
    if (storage.contains(e)) {
        if (! in_group) {
            // Remove from group
            storage.erase(e);
        }
    } else {
        if (in_group) {
            // Add to group
            storage.emplace(e);
        }
    }
}

extern "C" void* component_get_for_entity (scripting::Context* context, std::uint32_t entity, const char* component_name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto it = context->m_component_types.find(entt::hashed_string::value(component_name));
    if (it != context->m_component_types.end()) {
        auto& storage = registry.storage(it->second)->second; // `it` would not be valid if this storage doesn't exist
        entt::entity e = static_cast<entt::entity>(entity);
        if (storage.contains(e)) {
            return storage.get(e);
        }
        // Entity does not have component
        spdlog::warn("[script] entity {} does not have component: {}", entity, component_name);
    } else {
        // Invalid component name
        spdlog::error("[script] component_get_for_entity invalid component name: {}", component_name);
    }
    return nullptr;
}

extern "C" void* component_add_to_entity (scripting::Context* context, std::uint32_t entity, const char* component_name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto it = context->m_component_types.find(entt::hashed_string::value(component_name));
    if (it != context->m_component_types.end()) {
        auto& storage = registry.storage(it->second)->second; // `it` would not be valid if this storage doesn't exist
        entt::entity e = static_cast<entt::entity>(entity);
        if (!storage.contains(e)) {
            if (storage.emplace(e) == storage.end()) {
                // Could not insert component
                spdlog::warn("[script] component_add_to_entity with entity {} was unable to add component: {}", entity, component_name);
                return nullptr;
            }
        }
        return storage.get(e);
    }
    // Invalid component name
    spdlog::error("[script] component_add_to_entity invalid component name: {}", component_name);
    return nullptr;
}

extern "C" void component_tag_entity (scripting::Context* context, uint32_t entity, const char* tag_name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto& storage = registry.storage<void>(entt::hashed_string{tag_name});
    entt::entity e = static_cast<entt::entity>(entity);
    if (!storage.contains(e)) {
        spdlog::warn("Adding tag {} to {}", tag_name, entity);
        storage.emplace(e);
    }
}

extern "C" void component_remove_from_entity (scripting::Context* context, std::uint32_t entity, const char* component_name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::registry& registry = world::registry(context->m_world_ctx);
    auto it = context->m_component_types.find(entt::hashed_string::value(component_name));
    if (it != context->m_component_types.end()) {
        auto& storage = registry.storage(it->second)->second; // `it` would not be valid if this storage doesn't exist
        entt::entity e = static_cast<entt::entity>(entity);
        if (!storage.contains(e)) {
            storage.remove(e);
        } else {
            // Entity does not have component
            spdlog::warn("[script] component_remove_from_entity entity {} does not have component: {}", entity, component_name);
        }
    } else {
        // Invalid component name
        spdlog::error("[script] component_remove_from_entity invalid component name: {}", component_name);
    }
}

extern "C" bool is_in_group (scripting::Context* context, std::uint32_t entity, std::uint32_t group)
{
    return world::isInGroup(context->m_world_ctx, static_cast<entt::entity>(entity), entt::hashed_string::hash_type(group));
}

extern "C" std::uint32_t get_group (scripting::Context* context, std::uint32_t group, const std::uint32_t** entities)
{
    const auto& registry = world::registry(context->m_world_ctx);
    const auto& storage = registry.storage<core::EntityGroup>(entt::hashed_string::hash_type(group));
    *entities = reinterpret_cast<const uint32_t*>(storage.data());
    return storage.size();
}

extern "C" std::uint32_t get_entity_composite (scripting::Context* context, std::uint32_t, const std::uint32_t**)
{
    return 0;
}
