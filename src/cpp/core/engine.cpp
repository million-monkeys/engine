
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

bool core::Engine::execute (Time current_time, DeltaTime delta, uint64_t frame_count)
{
    EASY_FUNCTION(profiler::colors::Blue100);
    m_current_time_delta = delta;

    // Check if any resources are loaded
    resources::poll(this);

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
                {
                    auto& new_scene = eventData<events::engine::LoadScene>(ev);
                    m_scene_manager.loadScene(million::Registry::Background, new_scene.scene_id);
                    break;
                }
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

    for (const auto& ev : events()) {
        if (ev.type == "resource/loaded"_hs) {
            auto& loaded = eventData<events::engine::ResourceLoaded>(ev);
            if (loaded.name == "script2"_hs) {
                scripting::load("test.lua");
            }
        }
    }

    scripting::processEvents(*this);
    pumpEvents();

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

