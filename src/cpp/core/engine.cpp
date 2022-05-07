
#include "engine.hpp"
#include "graphics/graphics.hpp"
#include "resources/resources.hpp"
#include "scripting/scripting.hpp"

void destroyInputData (struct core::InputData*);

core::Engine::~Engine ()
{
    destroyInputData(m_input_data);
}

void core::Engine::readBinaryFile (const std::string& filename, std::string& buffer) const
{
    buffer = helpers::readToString(filename);
}

million::resources::Handle core::Engine::findResource (entt::hashed_string::hash_type name)
{
    auto it = m_named_resources.find(name);
    if (it != m_named_resources.end()) {
        spdlog::debug("Resource with name {:#x} found: {:#x}", name, it->second.handle);
        return it->second;
    }
    spdlog::debug("Resource with name {:#x} not found", name);
    return million::resources::Handle::invalid();
}

million::resources::Handle core::Engine::loadResource (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    auto handle = resources::load(type, filename, name);
    if (name != 0) {
        m_named_resources[name] = handle;
        spdlog::debug("Bound resource {:#x} to name {:#x}", handle.handle, name);
    }
    return handle;
}

void core::Engine::setGameState (entt::hashed_string new_state)
{
    spdlog::info("Setting game state to: {}", new_state.data());
    m_current_game_state = new_state;
}

bool core::Engine::execute (Time current_time, DeltaTime delta, uint64_t frame_count)
{
    EASY_FUNCTION(profiler::colors::Blue100);
    m_current_time_delta = delta;

    // Check if any resources are loaded
    resources::poll();

    // Check if a new scene has loaded
    m_scene_manager.update();

    // Read input device states and dispatch events. Input events are emitted directly into the global pool, immediately readable "this frame" (no frame delay!)
    handleInput();

    {
        EASY_BLOCK("Handling System Events", profiler::colors::Amber100);
        // Process previous frames events, looking for ones the core engine cares about
        // Yes, its a bit wasteful to loop them all like this, but they should be hot in cache so ¯\_(ツ)_/¯
        for (const auto& ev : events("commands"_hs)) {
            EASY_BLOCK("Handling event", profiler::colors::Amber200);
            switch (ev.type) {
                case commands::engine::Exit::ID:
                    // No longer running, return false.
                    return false;
                case "engine/set-system-status/running"_hs:
                    m_system_status = SystemStatus::Running;
                    break;
                case "engine/set-system-status/stopped"_hs:
                    m_system_status = SystemStatus::Stopped;
                    break;
                // case "scene/registry/runtime->background"_hs:
                //     copyRegistry(m_runtime_registry, m_background_registry);
                //     break;
                // case "scene/registry/background->runtime"_hs:
                //     {
                //         auto names = m_named_entities;
                //         copyRegistry(m_background_registry, m_runtime_registry);
                //         m_named_entities = names;
                //     }
                //     break;
                // case "scene/registry/clear-background"_hs:
                //     m_background_registry.clear();
                //     break;
                // case "scene/registry/clear-runtime"_hs:
                //     m_runtime_registry.clear();
                //     break;
                case commands::scenes::Load::ID:
                {
                    auto& new_scene = eventData<commands::scenes::Load>(ev);
                    m_scene_manager.loadScene(new_scene.scene_id, new_scene.auto_swap);
                    break;
                }
                default:
                    break;
            };
        }
        // for (const auto& ev : events("resources"_hs)) {
        //     EASY_BLOCK("Handling resource event", profiler::colors::Amber200);
        //     switch (ev.type) {
        //         case events::engine::ResourceLoaded::ID:
        //         {
        //             auto& loaded = eventData<events::engine::ResourceLoaded>(ev);
        //             spdlog::info("Resource loaded: {} ({})", loaded.name, loaded.name == "script1"_hs);
        //             if (loaded.name == "script1"_hs) {
        //                 scripting::load("test.lua");
        //             }
        //             if (loaded.name == "scene-script"_hs) {
        //                 spdlog::warn("Scene scripts loaded");
        //             }
        //             break;
        //         }
        //         default:
        //             break;
        //     };
        // }
    }

    // Run the before-frame hook for each module, updating the current time
    callModuleHook<CM::BEFORE_FRAME>(current_time, delta, frame_count);

    if (m_system_status == SystemStatus::Running) {
        // Execute the taskflow graph if systems are running
        m_scheduler.execute();
    } else {
        // If systems are stopped, only pump events
        pumpEvents();
    }

    // Run the after-frame hook for each module
    callModuleHook<CM::AFTER_FRAME>();

    /*
     * Hand over exclusive access to engine state to renderer.
     * Renderer will access the ECS registry to gather all components needed for rendering, accumulate
     * a render list and hand exclusive access back to the engine. The renderer wil then asynchronously
     * render from its locally owned render list.
     */
    // {
    //     EASY_BLOCK("Waiting on renderer", profiler::colors::Red100);
    //     // First, signal to the renderer that it has exclusive access to the engines state
    //     {
    //         std::scoped_lock<std::mutex> lock(m_graphics_sync->state_mutex);
    //         m_graphics_sync->owner = graphics::Sync::Owner::Renderer;
    //     }
    //     m_graphics_sync->sync_cv.notify_one();

    //     // Now wait for the renderer to relinquish exclusive access back to the engine
    //     std::unique_lock<std::mutex> lock(m_graphics_sync->state_mutex);
    //     m_graphics_sync->sync_cv.wait(lock, [this]{ return m_graphics_sync->owner == graphics::Sync::Owner::Engine; });
    // }

    /*
     * Engine has exclusive access again.
     */

    // Still running, return true.
    return true;
}

void core::Engine::executeHandlers (HandlerType type)
{
    auto& message_publisher = publisher();
    switch (type) {
    case HandlerType::Game:
        {
            EASY_BLOCK("Events/game", profiler::colors::Purple100);
            for (const auto& handler : m_game_handlers[m_current_game_state]) {
                handler(events("core"_hs), message_publisher);
            }
            break;
        }
    case HandlerType::Scene:
        {
            EASY_BLOCK("Events/scene", profiler::colors::Purple100);
            for (const auto& handler : m_scene_handlers[m_scene_manager.current()]) {
                handler(events("core"_hs), message_publisher);
            }
            break;
        }
    };
}

void core::Engine::shutdown ()
{
    // // Unload the current scene
    // callModuleHook<CM::UNLOAD_SCENE>();
    // // Shut down graphics thread
    // graphics::term(m_renderer);
    
    // Uninstall resources managers
    resources::term();
    // Halt the scripting system
    scripting::term();    
    // Delete message pools
    m_message_pool.reset();
    for (auto pool : m_message_pools) {
        delete pool;
    }
    m_message_pools.clear();
    // Clear the registries
    m_registries.background().clear();
    m_registries.foreground().clear();
}
