
#include "engine.hpp"


#include "engine.hpp"
#include "graphics/graphics.hpp"

#include <SDL.h>


core::Engine::~Engine ()
{

}

void core::Engine::readBinaryFile (const std::string& filename, std::string& buffer) const
{
    buffer = helpers::readToString(filename);
}

entt::registry& core::Engine::registry(monkeys::Registry which)
{
    switch (which) {
    case monkeys::Registry::Runtime:
        return m_runtime_registry;
    case monkeys::Registry::Background:
        return m_background_registry;
    case monkeys::Registry::Prototype:
        return m_prototype_registry;
    };
}

entt::organizer& core::Engine::organizer(monkeys::SystemStage type)
{
    return m_organizers[type];
}

entt::entity core::Engine::findEntity (entt::hashed_string name) const
{
    auto it = m_named_entities.find(name);
    if (it != m_named_entities.end()) {
        return it->second.entity;
    }
    return entt::null;
}

const std::string& core::Engine::findEntityName (const components::core::Named& named) const
{
    auto it = m_named_entities.find(named.name);
    if (it != m_named_entities.end()) {
        return it->second.name;
    } else {
        spdlog::warn("No name for {}", named.name.data());
    }
    return m_empty_string;
}

monkeys::resources::Handle core::Engine::findResource (entt::hashed_string::hash_type name)
{
    return {};
}


void core::Engine::loadComponent (entt::registry& registry, entt::hashed_string component, entt::entity entity, const void* table)
{
    EASY_FUNCTION(profiler::colors::Green100);
    auto it = m_component_loaders.find(component);
    if (it != m_component_loaders.end()) {
        const auto& loader = it->second;
        loader(this, registry, table, entity);
    } else {
        spdlog::warn("Tried to load non-existent component: {}", component.data());
    }
}

#if 0

void core::Engine::handleInput ()
{
    EASY_FUNCTION(profiler::colors::LightBlue100);
    /**
     * Gather input from input devices: keyboard, mouse, gamepad, joystick
     * Input is mapped to events and those events are emitted for systems to process.
     * handleInput doesn't use the engines normal emit() API, instead it adds the events
     * directly to the m_event_pool global event pool. This way, input-generated events
     * are immediately available, without a frame-delay. We can do this, because handleInput
     * is guaranteed to be called serially, before the taskflow graph is executed, so we can
     * guarantee 
     */

    SDL_Event event;
    m_input_events.clear();
    // Gather and dispatch input
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
                internalEmplaceEvent("engine/exit"_event);
                break;
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    entt::monostate<"graphics/resolution/width"_hs>{} = int(event.window.data1);
                    entt::monostate<"graphics/resolution/height"_hs>{} = int(event.window.data2);
                    graphics::windowChanged(m_renderer);
                    break;
                default:
                    break;
                };
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    internalEmplaceEvent("engine/exit"_event);
                } else {
                    // handleInput(engine, input_mapping, InputKeys::KeyType::KeyboardButton, event.key.keysym.scancode, [&event]() -> float {
                    //     return event.key.state == SDL_PRESSED ? 1.0f : 0.0f;
                    // });
                }
                break;
            }
            case SDL_CONTROLLERDEVICEADDED:
                // TODO: Handle multiple gamepads
                m_game_controller = SDL_GameControllerOpen(event.cdevice.which);
                if (m_game_controller == 0) {
                    spdlog::error("Could not open gamepad {}: {}", event.cdevice.which, SDL_GetError());
                } else {
                    spdlog::info("Gamepad detected {}", SDL_GameControllerName(m_game_controller));
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                SDL_GameControllerClose(m_game_controller);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            {
                // handleInput(engine, input_mapping, InputKeys::KeyType::ControllerButton, event.cbutton.button, [&event]() -> float {
                //     return event.cbutton.state ? 1.0f : 0.0f;
                // });
                break;
            }
            case SDL_CONTROLLERAXISMOTION:
            {
                // handleInput(engine, input_mapping, InputKeys::KeyType::ControllerAxis, event.caxis.axis, [&event]() -> float {
                //     float value = float(event.caxis.value) * Constants::GamePadAxisScaleFactor;
                //     if (std::fabs(value) < 0.15f /* TODO: configurable deadzones */) {
                //         return 0;
                //     }
                //     return value;
                // });
                break;
            }
            default:
                break;
        };
        // Store events for render thread to access (used by Dear ImGui)
        m_input_events.push_back(event);
    }

    // Make sure that any events that were dispatched are visible to the engine
    refreshEventsIterator();
}

bool core::Engine::execute (Time current_time, DeltaTime delta, uint64_t frame_count)
{
    EASY_FUNCTION(profiler::colors::Blue100);
    m_current_time_delta = delta;

    // Read input device states and dispatch events. Input events are emitted directly into the global pool, immediately readable "this frame" (no frame delay!)
    handleInput();

    // // Process previous frames events, looking for ones the core engine cares about
    // Yes, its a bit wasteful to loop them all like this, but they should be hot in cache so ¯\_(ツ)_/¯
    for (auto& event : helpers::const_iterate(events())) {
        EASY_BLOCK("Handling event", profiler::colors::Amber100);
        switch (event.type) {
            case "engine/exit"_event:
                return false;
            case "engine/set-system-status/running"_event:
                m_system_status = SystemStatus::Running;
                break;
            case "engine/set-system-status/stopped"_event:
                m_system_status = SystemStatus::Stopped;
                break;
            case "scene/registry/runtime->background"_event:
                copyRegistry(m_runtime_registry, m_background_registry);
                break;
            case "scene/registry/background->runtime"_event:
                {
                    auto names = m_named_entities;
                    copyRegistry(m_background_registry, m_runtime_registry);
                    m_named_entities = names;
                }
                break;
            case "scene/registry/clear-background"_event:
                m_background_registry.clear();
                break;
            case "scene/registry/clear-runtime"_event:
                m_runtime_registry.clear();
                break;
            case "scene/load"_event:
                m_scene_manager.loadScene(event.hash_value);
                break;
            default:
                break;
        };
    }

    // Run the before-frame hook for each module, updating the current time
    callModuleHook<CM::BEFORE_FRAME>(current_time, delta, frame_count);

    if (m_system_status == SystemStatus::Running) {
        // Execute the taskflow graph if systems are running
        EASY_BLOCK("Executing tasks", profiler::colors::Indigo200);
        m_executor.run(m_coordinator);
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
    {
        EASY_BLOCK("Waiting on renderer", profiler::colors::Red100);
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

    /*
     * Engine has exclusive access again.
     */

    // Still running, return true.
    return true;
}

#endif

void core::Engine::copyRegistry (const entt::registry& from, entt::registry& to)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    from.each([&from,&to](const auto source_entity) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            auto destination_storage = to.storage(id);
            if(destination_storage != nullptr && source_storage.contains(source_entity)) {
                destination_storage->emplace(destination_entity, source_storage.get(source_entity));
            }
        }
    });

}

void core::Engine::onAddNamedEntity (entt::registry& registry, entt::entity entity)
{
    const auto& named = registry.get<components::core::Named>(entity);
    m_named_entities[named.name] = {entity, named.name.data()};
}

void core::Engine::onRemoveNamedEntity (entt::registry& registry, entt::entity entity)
{
    const auto& named = registry.get<components::core::Named>(entity);
    m_named_entities.erase(named.name);
}

