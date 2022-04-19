
#include "core.hpp"
#include "core/engine.hpp"

// Script functions need access to an engine instance
core::Engine* g_engine = nullptr;

void set_engine (core::Engine* engine)
{
    g_engine = engine;
}

phmap::flat_hash_map<entt::hashed_string::hash_type, entt::id_type> g_component_types;

void scripting::registerComponent (entt::hashed_string::hash_type name, entt::id_type id)
{
    g_component_types[name] = id;
}

entt::registry dummy_registry;

/******************************************************************************
 * Functions exposed to scripts
 ******************************************************************************/

extern "C" std::uint32_t null_entity_value ()
{
    return magic_enum::enum_integer(entt::entity{entt::null});
}

extern "C" std::uint32_t entity_create (std::uint32_t which_registry)
{
    auto selected_registry = magic_enum::enum_cast<monkeys::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        return magic_enum::enum_integer(registry.create());
    } else {
        // Invalid registry
        spdlog::error("[script] entity_create called with invalid registry: {}", which_registry);
        return magic_enum::enum_integer(entt::entity{entt::null});
    }
}

extern "C" void entity_destroy (std::uint32_t which_registry, std::uint32_t entity)
{
    auto selected_registry = magic_enum::enum_cast<monkeys::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        registry.destroy(static_cast<entt::entity>(entity));
    } else {
        // Invalid registry
        spdlog::error("[script] entity_destroy called with invalid registry: {}", which_registry);
    }
}

extern "C" std::uint32_t entity_lookup_by_name (std::uint32_t which_registry, const char* name)
{
    return magic_enum::enum_integer(g_engine->findEntity(entt::hashed_string{name}));
}

extern "C" void* component_get_for_entity (std::uint32_t which_registry, std::uint32_t entity, const char* component_name)
{
    auto selected_registry = magic_enum::enum_cast<monkeys::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
            auto storage = registry.storage(it->second);
            entt::entity e = static_cast<entt::entity>(entity);
            if (storage->contains(e)) {
                return storage->get(e);
            }
            // Entity does not have component
            spdlog::warn("[script] entity {} does not have component: {}", entity, component_name);
        } else {
            // Invalid component name
            spdlog::error("[script] component_get_for_entity invalid component name: {}", component_name);
        }
        return nullptr;
    } else {
        // Invalid registry
        spdlog::error("[script] component_get_for_entity called with invalid registry: {}", which_registry);
        return nullptr;
    }
}

extern "C" void* component_add_to_entity (std::uint32_t which_registry, std::uint32_t entity, const char* component_name)
{
    auto selected_registry = magic_enum::enum_cast<monkeys::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
            auto storage = registry.storage(it->second);
            entt::entity e = static_cast<entt::entity>(entity);
            if (!storage->contains(e)) {
                if (storage->emplace(e) == storage->end()) {
                    // Could not insert component
                    spdlog::warn("[script] component_add_to_entity with entity {} was unable to add component: {}", entity, component_name);
                    return nullptr;
                }
            }
            return storage->get(e);
        }
        // Invalid component name
        spdlog::error("[script] component_add_to_entity invalid component name: {}", component_name);
        return nullptr;
    } else {
        // Invalid registry
        spdlog::error("[script] component_add_to_entity called with invalid registry: {}", which_registry);
        return nullptr;
    }
}

extern "C" void component_remove_from_entity (std::uint32_t which_registry, std::uint32_t entity, const char* component_name)
{
    auto selected_registry = magic_enum::enum_cast<monkeys::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
            auto storage = registry.storage(it->second);
            entt::entity e = static_cast<entt::entity>(entity);
            if (!storage->contains(e)) {
                storage->remove(e);
            } else {
                // Entity does not have component
                spdlog::warn("[script] component_remove_from_entity entity {} does not have component: {}", entity, component_name);
            }
        } else {
            // Invalid component name
            spdlog::error("[script] component_remove_from_entity invalid component name: {}", component_name);
        }
    } else {
        // Invalid registry
        spdlog::error("[script] component_remove_from_entity called with invalid registry: {}", which_registry);
    }
}

extern "C" void emit_event (const char* event_name, std::uint32_t sender, void* event)
{
    [[maybe_unused]] entt::hashed_string::hash_type event_type = entt::hashed_string::value(event_name);
    [[maybe_unused]] entt::entity entity = static_cast<entt::entity>(sender);
    // TODO: Copy `event` data into event stream
}

extern "C" void output_log (std::uint32_t level, const char* message)
{
    switch (level) {
    case 0:
        spdlog::critical("[script] {}", message);
        break;
    case 1:
        spdlog::error("[script] {}", message);
        break;
    case 2:
        spdlog::warn("[script] {}", message);
        break;
    case 3:
        spdlog::info("[script] {}", message);
        break;
    case 4:
        spdlog::debug("[script] {}", message);
        break;
    default:
        spdlog::warn("[script] invalid log level: {}", level);
        break;
    }
}
