
#include "engine.hpp"
#include "config.hpp"
#include "resources/resources.hpp"
#include "scripting/scripting.hpp"
#include "graphics/graphics.hpp"

#include <events/engine.hpp>

#include <entt/entity/organizer.hpp>
#include <entt/entity/view.hpp>

struct core::InputData* createInputData (); // Just to avoid having to include SDL.h from engine.hpp

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
    m_stream_sizes(stream_sizes),
    m_message_pool(get_global_event_pool_size()),
    m_commands(createStream("commands"_hs, million::StreamWriters::Multi)),
    m_scene_manager(*this),
    m_input_data(createInputData())
{
    createEngineStream("game"_hs);
    createEngineStream("scene"_hs);
    createEngineStream("behavior"_hs);
    setContextData();
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

void core::Engine::registerGameHandler (entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler)
{
    m_game_handlers[state].push_back({events, handler});
}

void core::Engine::registerSceneHandler (entt::hashed_string scene, entt::hashed_string::hash_type events, million::SceneHandler handler)
{
    m_scene_handlers[scene].push_back({events, handler});
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

void core::Engine::installComponent (const million::api::definitions::Component& component, million::api::definitions::PrepareFn prepareFn)
{
    // Create storage for the component
    prepareFn(m_registries.foreground().runtime);
    prepareFn(m_registries.foreground().prototypes);
    prepareFn(m_registries.background().runtime);
    prepareFn(m_registries.background().prototypes);
    // Make component accessible from scripting
    scripting::registerComponent(component.id.value(), component.type_id);
    // Add component loader
    m_component_loaders[component.id] = component.loader;
}

bool core::Engine::init ()
{
    EASY_FUNCTION(profiler::colors::Pink50);
    init_core::register_components(this);
    resources::init(this);
    if (! scripting::init(this)) {
        spdlog::critical("Could not initialize scripting subsystem");
        return false;
    }
    m_system_status = SystemStatus::Running;

    m_graphics_sync = graphics::init(*this);
    if (m_graphics_sync == nullptr) {
        return false;
    }
    // Sync with graphics to make sure render thread is set up before continuing
    {
        // First, signal to the renderer that it has exclusive access to the engines state
        {
            std::scoped_lock<std::mutex> lock(m_graphics_sync->state_mutex);
            m_graphics_sync->owner = graphics::Sync::Owner::Renderer;
        }
        m_graphics_sync->sync_cv.notify_one();
        // Now wait for the renderer to relinquish exclusive access back to the engine
        std::unique_lock<std::mutex> lock(m_graphics_sync->state_mutex);
        m_graphics_sync->sync_cv.wait(lock, [this]{ return m_graphics_sync->owner == graphics::Sync::Owner::Engine; });
    }
        
    return true;
}

bool core::Engine::setupGame ()
{
    EASY_FUNCTION(profiler::colors::Pink50);
    // Read game configuration
    helpers::hashed_string_flat_map<std::string> game_scripts;
    std::vector<entt::hashed_string::hash_type> entity_categories;
    if (!core::readGameConfig(game_scripts, entity_categories)) {
        return false;
    }

    // Add entity categories
    std::uint16_t bit_index = 0;
    for (const auto& category : entity_categories) {
        m_category_bitfields.emplace(category, 1 << bit_index++);
    }

    // Set up game scenes
    const std::string& scene_path = entt::monostate<"scenes/path"_hs>();
    m_scene_manager.loadSceneList(scene_path);

    // Make sure game-specific events are declared in Lua
    scripting::load(entt::monostate<"game/script-events"_hs>());

    // Create game task graph
    m_scheduler.createTaskGraph(*this);

    // load initial scene
    m_commands.emit<commands::scenes::Load>([](auto& load){
        const std::string initial_scene = entt::monostate<"scenes/initial"_hs>();
        load.scene_id = entt::hashed_string::value(initial_scene.c_str());
        load.auto_swap = true;
    });

    // Set up game state
    const std::string& start_state_str = entt::monostate<"game/initial-state"_hs>();
    auto start_state = entt::hashed_string{start_state_str.c_str()};
    setGameState(start_state);

    // Load game event handler scripts
    for (const auto& [game_state, script_file] :  game_scripts) {
        if (game_state == start_state) {
            m_system_status = SystemStatus::Loading;
        }
        loadResource("game-script"_hs, script_file, game_state);
    }

    // Make events emitted during load visible on first frame
    pumpEvents();

    return true;
}
