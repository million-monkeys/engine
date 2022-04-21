
#include "engine.hpp"
#include "scripting/core.hpp"

namespace init_core {
    void register_components (monkeys::api::Engine*);
}

// Taskflow workers = number of hardware threads - 1, unless there is only one hardware thread
int get_num_workers () {
    auto max_workers = std::thread::hardware_concurrency();
    return max_workers > 1 ? max_workers - 1 : max_workers;
}

int get_global_event_pool_size () {
    const std::uint32_t pool_size = entt::monostate<"memory/events/pool-size"_hs>();
    return pool_size * get_num_workers();
}

core::Engine::Engine() :
    m_scene_manager(*this),
    m_events_iterable(nullptr, nullptr),
    m_event_pool(get_global_event_pool_size())
{
    // Manage Named entities
    m_runtime_registry.on_construct<components::core::Named>().connect<&core::Engine::onAddNamedEntity>(this);
    m_runtime_registry.on_destroy<components::core::Named>().connect<&core::Engine::onRemoveNamedEntity>(this);
    // Manage prototype entities
    m_prototype_registry.on_construct<core::EntityPrototypeID>().connect<&core::Engine::onAddPrototypeEntity>(this);
    m_prototype_registry.on_destroy<core::EntityPrototypeID>().connect<&core::Engine::onRemovePrototypeEntity>(this);
}

void* core::Engine::allocModule (std::size_t bytes) {
    return reinterpret_cast<void*>(new std::byte[bytes]);
}

void core::Engine::deallocModule (void* ptr) {
    delete [] reinterpret_cast<std::byte*>(ptr);
}

void core::Engine::installComponent (const monkeys::api::definitions::Component& component)
{
    scripting::registerComponent(component.id.value(), component.type_id);
}

bool core::Engine::init ()
{
    init_core::register_components(this);
    if (!scripting::init(this)) {
        spdlog::critical("Could not initialize scripting subsystem");
        return false;
    }
    return true;
}

void core::Engine::reset ()
{
    // // Unload the current scene
    // callModuleHook<CM::UNLOAD_SCENE>();
    // // Shut down graphics thread
    // graphics::term(m_renderer);
    // // Delete event pools
    // for (auto pool : m_event_pools) {
    //     delete pool;
    // }
    // Halt the scripting system
    scripting::term();
    // Clear the runtime registry
    m_runtime_registry = {};
    // Clear background registry
    m_background_registry = {};
    // Clear the prototype registry
    m_prototype_registry = {};
}