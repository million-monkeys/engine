
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

void core::Engine::registerModule (std::uint32_t flags, monkeys::api::Module* mod)
{
    // All modules have the before-frame hook registered
    addModuleHook(CM::BEFORE_FRAME, mod);

    // Register optional hooks
    for (auto hook : {CM::AFTER_FRAME, CM::LOAD_SCENE, CM::UNLOAD_SCENE, CM::BEFORE_UPDATE, CM::PREPARE_RENDER, CM::BEFORE_RENDER, CM::AFTER_RENDER}) {
        if (flags & magic_enum::enum_integer(hook)) {
            addModuleHook(hook, mod);
        }
    }
}

void core::Engine::addModuleHook (monkeys::api::Module::CallbackMasks hook, monkeys::api::Module* mod) {
    switch (hook) {
        case CM::BEFORE_FRAME:
            m_hooks_beforeFrame.push_back(mod);
            break;
        case CM::AFTER_FRAME:
            m_hooks_afterFrame.push_back(mod);
            break;
        case CM::BEFORE_UPDATE:
            m_hooks_beforeUpdate.push_back(mod);
            break;
        case CM::PREPARE_RENDER:
            m_hooks_prepareRender.push_back(mod);
            break;
        case CM::BEFORE_RENDER:
            m_hooks_beforeRender.push_back(mod);
            break;
        case CM::AFTER_RENDER:
            m_hooks_afterRender.push_back(mod);
            break;
        case CM::LOAD_SCENE:
            m_hooks_loadScene.push_back(mod);
            break;
        case CM::UNLOAD_SCENE:
            m_hooks_unloadScene.push_back(mod);
            break;
    };
}

void core::Engine::installComponent (const monkeys::api::definitions::Component& component)
{
    scripting::registerComponent(component.id.value(), component.type_id);
}

bool core::Engine::init ()
{
    init_core::register_components(this);
    if (! scripting::init(*this)) {
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