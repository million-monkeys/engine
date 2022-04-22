
#include "engine.hpp"


#include "engine.hpp"
#include "graphics/graphics.hpp"

#include "resources/resources.hpp"

core::Engine::~Engine ()
{

}

void core::Engine::readBinaryFile (const std::string& filename, std::string& buffer) const
{
    buffer = helpers::readToString(filename);
}

monkeys::resources::Handle core::Engine::findResource (entt::hashed_string::hash_type name)
{
    return {};
}

struct TestEvent {
    static constexpr entt::hashed_string EventID = "test-event"_hs;
    int a;
    int b;
};

bool core::Engine::execute (Time current_time, DeltaTime delta, uint64_t frame_count)
{
    EASY_FUNCTION(profiler::colors::Blue100);
    m_current_time_delta = delta;

    // Read input device states and dispatch events. Input events are emitted directly into the global pool, immediately readable "this frame" (no frame delay!)
    handleInput();

    // // Process previous frames events, looking for ones the core engine cares about
    // Yes, its a bit wasteful to loop them all like this, but they should be hot in cache so ¯\_(ツ)_/¯
    for (const auto& ev : events()) {
        EASY_BLOCK("Handling event", profiler::colors::Amber100);
        switch (ev.type) {
            case "engine/exit"_hs:
                return false;
            case "engine/set-system-status/running"_hs:
                m_system_status = SystemStatus::Running;
                break;
            case "engine/set-system-status/stopped"_hs:
                m_system_status = SystemStatus::Stopped;
                break;
            case "scene/registry/runtime->background"_hs:
                copyRegistry(m_runtime_registry, m_background_registry);
                break;
            case "scene/registry/background->runtime"_hs:
                {
                    auto names = m_named_entities;
                    copyRegistry(m_background_registry, m_runtime_registry);
                    m_named_entities = names;
                }
                break;
            case "scene/registry/clear-background"_hs:
                m_background_registry.clear();
                break;
            case "scene/registry/clear-runtime"_hs:
                m_runtime_registry.clear();
                break;
            case "scene/load"_hs:
                // TODO: Create scene load event
                // m_scene_manager.loadScene(event.hash_value);
                break;
            default:
                break;
        };
    }

    // Run the before-frame hook for each module, updating the current time
    callModuleHook<CM::BEFORE_FRAME>(current_time, delta, frame_count);

    // if (m_system_status == SystemStatus::Running) {
    //     // Execute the taskflow graph if systems are running
    //     EASY_BLOCK("Executing tasks", profiler::colors::Indigo200);
    //     m_executor.run(m_coordinator);
    // } else {
    //     // If systems are stopped, only pump events
    //     pumpEvents();
    // }

    ScriptedEvents se_resource;

    if (!se_resource.load("script1"_hs, "script1.res.toml")) {
        spdlog::error("Could not load resource: script1");
    }
    if (!se_resource.load("script2"_hs, "script2.res.toml")) {
        spdlog::error("Could not load resource: script1");
    }

    scripting::load("test.lua");
    pumpEvents();

    for (const auto& ev : events()) {
        if (ev.type == "test-event"_hs) {
            auto& test_event = event<TestEvent>(ev);
            spdlog::info("Got test-event: {}, {}", test_event.a, test_event.b);
        }
    }

    scripting::processEvents(*this);

    se_resource.unload("script1"_hs);
    se_resource.unload("script2"_hs);

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

