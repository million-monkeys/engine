
#include "engine.hpp"
#include "resources/resources.hpp"
#include "scripting/scripting.hpp"
#include <events/engine.hpp>

#include <entt/entity/organizer.hpp>
#include <entt/entity/view.hpp>

struct core::InputData* createInputData (); // Just to avoid having to include SDL.h from engine.hpp
extern core::EventPool* g_scripts_event_pool[2];

namespace init_core {
    void register_components (million::api::internal::ModuleManager*);
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

core::Engine::Engine(helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes) :
    m_scene_manager(*this),
    m_stream_sizes(stream_sizes),
    m_event_pool(get_global_event_pool_size()),
    m_commands(createStream("commands"_hs, million::StreamWriters::Multi)),
    m_input_data(createInputData())
{
    g_scripts_event_pool[0] = new core::EventPool(get_scripts_event_pool_size());
    g_scripts_event_pool[1] = new core::EventPool(get_scripts_event_pool_size());
    // Make sure that the pool addresses are stable by reserving enough space that they never have to be relocated
    m_event_pools.reserve(std::thread::hardware_concurrency() * 2); // Twice number of cores = maximum assumed threads (Total number of threads = taskflow workers + async resource loader + render thread + async background worker thread; No other threads should be created that use event pools)
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

void core::Engine::registerGameHandler (entt::hashed_string state, million::GameHandler handler)
{
    m_game_handlers[state].push_back(handler);
}

void core::Engine::registerSceneHandler (entt::hashed_string scene, million::SceneHandler handler)
{
    m_scene_handlers[scene].push_back(handler);
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
    m_component_loaders[component.id] = component.loader;
    // m_component_definitions.push_back(component_def);
}

bool core::Engine::init ()
{
    init_core::register_components(this);
    resources::init(this);
    if (! scripting::init(this)) {
        spdlog::critical("Could not initialize scripting subsystem");
        return false;
    }
    m_system_status = SystemStatus::Running;
        
    return true;
}

struct TestA {
    int a;
};
struct TestB {
    int a;
};
struct TestC {
    int a;
};

using Tag = void;

class TestSystem
{
public:
    void foo (const entt::registry& registry, entt::view<entt::get_t<TestA>> view)
    {
        const auto& my_tags = registry.storage<Tag>("my-tag"_hs);
        view.each([&my_tags, this](entt::entity e, auto& test){
            spdlog::debug("TestA.a: {} (tagged? {}), entity: {}", test.a, my_tags.contains(e) ? "yes" : "no", magic_enum::enum_integer(e));
            test.a += 1;
            if (a++ > 10) {
                spdlog::error("Quitting");
                m_commands->emit("engine/exit"_hs);
            }
        });
    }
    void bar (entt::view<entt::get_t<TestB>> view)
    {
        view.each([](auto& test){
            spdlog::debug("TestB.a: {}", test.a);
            test.a += 1;
        });
    }
    void baz (entt::view<entt::get_t<TestC>> view)
    {
        view.each([](auto& test){
            spdlog::debug("TestC.a: {}", test.a);
            test.a += 1;
        });
    }

    million::events::Stream* m_commands;
    int a = 0;
};

TestSystem g_test;

void core::Engine::setupGame ()
{
    g_test.m_commands = &commandStream();

    auto& o = organizer(million::SystemStage::GameLogic);
    o.emplace<&TestSystem::foo, Tag>(g_test, "A");
    o.emplace<&TestSystem::bar, Tag>(g_test, "B");
    o.emplace<&TestSystem::baz>(g_test, "C");
    // A and B in serial, in parallel with C

    // Create game task graph
    m_scheduler.createTaskGraph(*this);

    // Set up game scenes
    const std::string& scene_path = entt::monostate<"scenes/path"_hs>();
    m_scene_manager.loadSceneList(scene_path);

    m_commands.emit<events::engine::LoadScene>([](auto& load){
        const std::string initial_scene = entt::monostate<"scenes/initial"_hs>();
        load.scene_id = entt::hashed_string::value(initial_scene.c_str());
    });

    // Set up game state
    const std::string& start_state = entt::monostate<"game/initial-state"_hs>();
    setGameState(entt::hashed_string{start_state.c_str()});

    // Make events emitted during load visible on first frame
    pumpEvents();

    loadResource("scripted-events"_hs, "resources/script1.toml", "script1"_hs);
    loadResource("scripted-events"_hs, "resources/script2.toml", "script2"_hs);

    auto e = m_runtime_registry.create();
    m_runtime_registry.emplace<TestA>(e, 10);
    m_runtime_registry.emplace<TestB>(e, 10);
    m_runtime_registry.emplace<TestC>(e, 10);
    auto x = m_runtime_registry.create();
    m_runtime_registry.emplace<components::core::Named>(x, "test"_hs);
    m_runtime_registry.emplace<TestA>(x, 1000);

}
