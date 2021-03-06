#include "input.hpp"
#include "context.hpp"

void input::poll (input::Context* context)
{
    EASY_BLOCK("input::poll", input::COLOR(1));
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
    // m_input_events.clear();
    // Gather and dispatch input
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
                context->m_commands.emit("engine/exit"_hs);
                break;
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    entt::monostate<"graphics/resolution/width"_hs>{} = int(event.window.data1);
                    entt::monostate<"graphics/resolution/height"_hs>{} = int(event.window.data2);
                    // graphics::windowChanged(m_renderer);
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
                    context->m_commands.emit("engine/exit"_hs);
                } else {
                    // handleInput(engine, input_mapping, InputKeys::KeyType::KeyboardButton, event.key.keysym.scancode, [&event]() -> float {
                    //     return event.key.state == SDL_PRESSED ? 1.0f : 0.0f;
                    // });
                }
                break;
            }
            case SDL_CONTROLLERDEVICEADDED:
                // TODO: Handle multiple gamepads
                context->m_game_controller = SDL_GameControllerOpen(event.cdevice.which);
                if (context->m_game_controller == nullptr) {
                    spdlog::error("Could not open gamepad {}: {}", event.cdevice.which, SDL_GetError());
                } else {
                    spdlog::info("Gamepad detected {}", SDL_GameControllerName(context->m_game_controller));
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                SDL_GameControllerClose(context->m_game_controller);
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
        // m_input_events.push_back(event);
    }
}