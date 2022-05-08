
#include "scripting.hpp"
#include "core/engine.hpp"

#include <stdexcept>

// Script functions need access to an engine instance
core::Engine* g_engine = nullptr;

million::events::Stream* g_active_stream;

void set_active_stream (entt::hashed_string stream_name)
{
    g_active_stream = g_engine->engineStream(stream_name);
}

void init_scripting_api (core::Engine* engine)
{
    g_engine = engine;
}

phmap::flat_hash_map<entt::hashed_string::hash_type, entt::id_type, helpers::Identity> g_component_types;

void scripting::registerComponent (entt::hashed_string::hash_type name, entt::id_type id)
{
    g_component_types[name] = id;
}

/******************************************************************************
 * Functions exposed to scripts
 ******************************************************************************/

extern "C" std::uint32_t null_entity_value ()
{
    return magic_enum::enum_integer(entt::entity{entt::null});
}

extern "C" std::uint32_t entity_create (std::uint32_t which_registry)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        return magic_enum::enum_integer(registry.create());
    } else {
        // Invalid registry
        spdlog::error("[script] entity_create called with invalid registry: {}", which_registry);
        return magic_enum::enum_integer(entt::entity{entt::null});
    }
}

extern "C" std::uint32_t entity_create_from_prototype (std::uint32_t which_registry, const char* prototype)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        return magic_enum::enum_integer(g_engine->loadEntity(selected_registry.value(), entt::hashed_string{prototype}));
    } else {
        // Invalid registry
        spdlog::error("[script] entity_create called with invalid registry: {}", which_registry);
        return magic_enum::enum_integer(entt::entity{entt::null});
    }
}

extern "C" void entity_destroy (std::uint32_t which_registry, std::uint32_t entity)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
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
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
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
    } else {
        // Invalid registry
        spdlog::error("[script] component_get_for_entity called with invalid registry: {}", which_registry);
        return nullptr;
    }
}

extern "C" void* component_add_to_entity (std::uint32_t which_registry, std::uint32_t entity, const char* component_name)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
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
    } else {
        // Invalid registry
        spdlog::error("[script] component_add_to_entity called with invalid registry: {}", which_registry);
        return nullptr;
    }
}

extern "C" void component_tag_entity (uint32_t which_registry, uint32_t entity, const char* tag_name)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto& storage = registry.storage<void>(entt::hashed_string{tag_name});
        entt::entity e = static_cast<entt::entity>(entity);
        if (!storage.contains(e)) {
            spdlog::warn("Adding tag {} to {}", tag_name, entity);
            storage.emplace(e);
        }
    } else {
        // Invalid registry
        spdlog::error("[script] component_add_to_entity called with invalid registry: {}", which_registry);
    }
}

extern "C" void component_remove_from_entity (std::uint32_t which_registry, std::uint32_t entity, const char* component_name)
{
    auto selected_registry = magic_enum::enum_cast<million::Registry>(which_registry);
    if (selected_registry.has_value()) {
        entt::registry& registry = g_engine->registry(selected_registry.value());
        auto it = g_component_types.find(entt::hashed_string::value(component_name));
        if (it != g_component_types.end()) {
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
    } else {
        // Invalid registry
        spdlog::error("[script] component_remove_from_entity called with invalid registry: {}", which_registry);
    }
}

extern "C" void* allocate_message (const char* message_name, std::uint32_t target_entity, std::uint8_t size)
{
    entt::hashed_string::hash_type message_type = entt::hashed_string::value(message_name);
    entt::entity target = static_cast<entt::entity>(target_entity);
    return g_engine->publisher().push(message_type, target, size);
}

extern "C" void* allocate_command (const char* event_name, uint8_t size)
{
    entt::hashed_string::hash_type event_type = entt::hashed_string::value(event_name);
    return core::RawAccessWrapper(g_engine->commandStream()).push(event_type, size);
}

extern "C" void* allocate_event (const char* event_name, uint8_t size)
{
    entt::hashed_string::hash_type event_type = entt::hashed_string::value(event_name);
    return core::RawAccessWrapper(*g_active_stream).push(event_type, size);
}

extern "C" std::uint32_t get_messages (const char** buffer)
{
    g_engine->pumpMessages();
    auto iterable = g_engine->messages();
    auto& first = *iterable.begin();
    *buffer = reinterpret_cast<const char*>(&first);
    return iterable.size();
}

extern "C" std::uint32_t get_stream_events (std::uint32_t stream_name, const char** buffer)
{
    auto iterable = g_engine->events(stream_name);
    auto& first = *iterable.begin();
    *buffer = reinterpret_cast<const char*>(&first);
    return iterable.size();
}

struct BehaviorIterator {
    using const_iterable = typename entt::storage_traits<entt::entity, components::core::ScriptedBehavior>::storage_type::const_iterable;
    const_iterable::iterator next;
    const_iterable::iterator end;
    static BehaviorIterator* start (const_iterable);
    void cleanup ();
};

// memory::homogeneous::BitsetPool<BehaviorIterator, 4> g_iterator_pool;
BehaviorIterator g_iterator;
BehaviorIterator* BehaviorIterator::start (BehaviorIterator::const_iterable iter)
{
    // return g_iterator_pool.emplace(iter.begin(), iter.end());
    g_iterator = {iter.begin(), iter.end()};
    return &g_iterator;
}
void BehaviorIterator::cleanup ()
{
    // g_iterator_pool.discard(iter);
}

extern "C" BehaviorIterator* setup_scripted_behavior_iterator ()
{
    const auto& registry = g_engine->registry(million::Registry::Runtime);
    const auto& storage = registry.storage<components::core::ScriptedBehavior>();
    return BehaviorIterator::start(storage.each());
}

extern "C" std::uint32_t get_next_scripted_behavior (BehaviorIterator* iter, const components::core::ScriptedBehavior** behavior)
{
    if (iter->next != iter->end) {
        const auto&& [entity, sb] = *iter->next;
        ++iter->next;
        *behavior = &sb;
        return magic_enum::enum_integer(entity);
    } else {
        iter->cleanup();    
        *behavior = nullptr;
        return magic_enum::enum_integer(entt::entity{entt::null});
    }
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
    {
        spdlog::warn("[script] {}", message);
        const bool& terminate_on_error = entt::monostate<"engine/terminate-on-error"_hs>();
        if (terminate_on_error) {
            throw std::runtime_error("Script Error");
        }
        break;
    }
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

extern "C" std::uint32_t get_ref (const char* name)
{
    return entt::hashed_string::value(name);
}

extern "C" std::uint32_t load_resource (const char* type, const char* filename, const char* name)
{
    auto handle = g_engine->loadResource(entt::hashed_string{type}, std::string{filename}, name != nullptr ? entt::hashed_string::value(name) : 0);
    return handle.handle;
}

extern "C" std::uint32_t find_resource (const char* name)
{
    auto handle = g_engine->findResource(entt::hashed_string::value(name));
    return handle.handle;
}
