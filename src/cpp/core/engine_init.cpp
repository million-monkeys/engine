
#include "engine.hpp"
#include "resources/resources.hpp"
#include "scripting/scripting.hpp"

#include <entt/entity/organizer.hpp>
#include <entt/entity/view.hpp>

struct core::InputData* createInputData (); // Just to avoid having to include SDL.h from engine.hpp

namespace init_core {
    void register_components (million::api::Engine*);
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

int get_scripts_event_pool_size () {
    const std::uint32_t pool_size = entt::monostate<"memory/events/scripts-pool-size"_hs>();
    return pool_size;
}

core::Engine::Engine() :
    m_scripts_event_pool(get_scripts_event_pool_size()),
    m_scene_manager(*this),
    m_event_pool(get_global_event_pool_size()),
    m_input_data(createInputData())
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

void core::Engine::registerModule (std::uint32_t flags, million::api::Module* mod)
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

void core::Engine::addModuleHook (million::api::Module::CallbackMasks hook, million::api::Module* mod) {
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

void core::Engine::installComponent (const million::api::definitions::Component& component)
{
    scripting::registerComponent(component.id.value(), component.type_id);
}

bool core::Engine::init ()
{
    init_core::register_components(this);
    resources::init();
    if (! scripting::init(this)) {
        spdlog::critical("Could not initialize scripting subsystem");
        return false;
    }
    m_system_status = SystemStatus::Running;
        
    return true;
}

struct Test {
    int a;
};

class TestSystem
{
public:
    void foo (entt::view<entt::get_t<Test>> view)
    {
        view.each([](auto& test){
            spdlog::debug("Test.a: {}", test.a);
            test.a += 1;
        });
    }
};

TestSystem g_test;

void core::Engine::setupGame ()
{
    auto& o = organizer(million::SystemStage::GameLogic);
    o.emplace<&TestSystem::foo>(g_test, "test");

    m_scheduler.createTaskGraph(*this);

    // Make events emitted during load visible on first frame
    pumpEvents();

    loadResource("scripted-events"_hs, "resources/script1.toml", "script1"_hs);
    loadResource("scripted-events"_hs, "resources/script2.toml", "script2"_hs);

    m_runtime_registry.emplace<Test>(m_runtime_registry.create(), 10);
    m_runtime_registry.emplace<Test>(m_runtime_registry.create(), 1000);
}

void core::Engine::shutdown ()
{
    // // Unload the current scene
    // callModuleHook<CM::UNLOAD_SCENE>();
    // // Shut down graphics thread
    // graphics::term(m_renderer);
    
    // Halt the scripting system
    scripting::term();
    // Uninstall resources managers
    resources::term();
    // Delete event pools
    m_event_pool.reset();
    m_scripts_event_pool.reset();
    m_event_pools.clear();
    // Clear the runtime registry
    m_runtime_registry = {};
    // Clear background registry
    m_background_registry = {};
    // Clear the prototype registry
    m_prototype_registry = {};
}