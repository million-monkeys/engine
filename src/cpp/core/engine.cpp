#include "engine.hpp"

#include "utils/timekeeping.hpp"
#include "config/config.hpp"

#include "events/events.hpp"
#include "messages/messages.hpp"
#include "modules/modules.hpp"
#include "resources/resources.hpp"
#include "input/input.hpp"
#include "scripting/scripting.hpp"
#include "world/world.hpp"
#include "game/game.hpp"
#include "scheduler/scheduler.hpp"
#include "graphics/graphics.hpp"

namespace init_core {
    void register_components (million::api::internal::ModuleManager*);
}

bool Engine::init (std::shared_ptr<spdlog::logger> logger)
{
    EASY_BLOCK("Engine::init", Engine::COLOR(1));
    SPDLOG_DEBUG("[Engine] Init");
    // Load the game-specific settings
    if (! config::readEngineConfig()) {
        return false;
    }

    // Setup subsystems
    m_events_ctx = events::init();
    m_messages_ctx = messages::init();
    m_modules_ctx = modules::init(logger);
    m_resources_ctx = resources::init(m_events_ctx);
    m_input_ctx = input::init(m_events_ctx);
    m_scripting_ctx = scripting::init(m_messages_ctx, m_events_ctx, m_resources_ctx);
    if (m_scripting_ctx == nullptr) {
        return false;
    }
    m_world_ctx = world::init(m_events_ctx, m_messages_ctx, m_resources_ctx, m_scripting_ctx, m_modules_ctx);
    m_game_ctx = game::init(m_events_ctx, m_messages_ctx, m_world_ctx, m_scripting_ctx, m_resources_ctx, m_modules_ctx);
    m_scheduler_ctx = scheduler::init(m_world_ctx, m_scripting_ctx, m_events_ctx, m_game_ctx, m_modules_ctx);
    m_graphics_ctx = graphics::init(m_world_ctx, m_input_ctx, m_modules_ctx);
    if (m_graphics_ctx == nullptr) {
        return false;
    }

    // Scripting and world have a circular dependency...
    scripting::setWorld(m_scripting_ctx, m_world_ctx);
    // game and scheduler have a circular dependency...
    game::setScheduler(m_game_ctx, m_scheduler_ctx);

    // Setup module API by creating API wrapper objects
    modules::setup_api(m_modules_ctx, m_world_ctx, m_game_ctx, m_resources_ctx, m_scheduler_ctx, m_messages_ctx, m_events_ctx);

    // Register core components
    init_core::register_components(modules::api_module_manager(m_modules_ctx));

    // Setup game
    auto status = game::setup(m_game_ctx);
    if (! status.has_value()) {
        return false;
    }
    scheduler::setStatus(m_scheduler_ctx, status.value());

    // Initialized successfully
    return true;
}

void Engine::shutdown ()
{
    EASY_BLOCK("Engine::shutdown", Engine::COLOR(1));
    SPDLOG_DEBUG("[Engine] Shutdown");
    // Terminate game and world before unloading modules
    if (m_game_ctx) {
        game::term(m_game_ctx);
    }
    if (m_world_ctx) {
        world::term(m_world_ctx);
    }
    // Now that the registries are cleared, unload moudles
    modules::unload(m_modules_ctx);
    // Then terminate the rest of the subsystems
    if (m_graphics_ctx) {
        graphics::term(m_graphics_ctx);
    }
    if (m_scheduler_ctx) {
        scheduler::term(m_scheduler_ctx);
    }
    if (m_scripting_ctx) {
        scripting::term(m_scripting_ctx);
    }
    if (m_input_ctx) {
        input::term(m_input_ctx);
    }
    if (m_resources_ctx) {
        resources::term(m_resources_ctx);
    }
    if (m_modules_ctx) {
        modules::term(m_modules_ctx);
    }
    if (m_messages_ctx) {
        messages::term(m_messages_ctx);
    }
    if (m_events_ctx) {
        events::term(m_events_ctx);
    }
}

void Engine::execute ()
{
    timekeeping::FrameTimer frame_timer;

    // Run main loop
    spdlog::info("Game Running...");
    do {
        EASY_BLOCK("Execute", Engine::COLOR(2));
        // // Execute systems and copy current frames events for processing next frame
        auto current_time = frame_timer.sinceStart();
        auto delta = frame_timer.frameTime();
        auto frame_count = frame_timer.totalFrames();

        scripting::call(m_scripting_ctx, "set_game_time", delta, current_time);

        // Check if any resources are loaded
        resources::poll(m_resources_ctx);

        // Check if a new scene has loaded
        world::update(m_world_ctx);

        {
            EASY_BLOCK("Handling System Events", Engine::COLOR(2));
            // Process previous frames events, looking for ones the core engine cares about
            // Yes, its a bit wasteful to loop them all like this, but they should be hot in cache so ¯\_(ツ)_/¯
            for (const auto& ev : events::events(m_events_ctx, "commands"_hs)) {
                EASY_BLOCK("Handling event", Engine::COLOR(3));
                switch (ev.type) {
                    case commands::engine::Exit::ID:
                        SPDLOG_TRACE("[core] Got EXIT command");
                        // No longer running, return.
                        return;
                    case "engine/set-system-status/running"_hs:
                        scheduler::setStatus(m_scheduler_ctx, scheduler::SystemStatus::Running);
                        break;
                    case "engine/set-system-status/stopped"_hs:
                        scheduler::setStatus(m_scheduler_ctx, scheduler::SystemStatus::Stopped);
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
                    case commands::scene::Load::ID:
                    {
                        auto& new_scene = million::api::EngineRuntime::eventData<commands::scene::Load>(ev);
                        world::loadScene(m_world_ctx, new_scene.scene_id, new_scene.auto_swap);
                        break;
                    }
                    default:
                        break;
                };
            }
        }

        // Let game process
        game::execute(m_game_ctx, current_time, delta, frame_count);

        // Run the before-frame hook for each module, updating the current time
        modules::hooks::before_frame(m_modules_ctx, current_time, delta, frame_count);

        // Call scheduler to run tasks
        if EXPECT_NOT_TAKEN(! scheduler::execute(m_scheduler_ctx)) {
            return;
        }

        // Run the after-frame hook for each module
        modules::hooks::after_frame(m_modules_ctx);

        /*
        * Hand over exclusive access to engine state to renderer.
        * Renderer will access the ECS registry to gather all components needed for rendering, accumulate
        * a render list and hand exclusive access back to the engine. The renderer wil then asynchronously
        * render from its locally owned render list.
        */
        graphics::handOff(m_graphics_ctx);
        /*
        * Engine has exclusive access again.
        */

        // Update timekeeping
        frame_timer.update();

    } while (true);
    frame_timer.reportAverage();
}
