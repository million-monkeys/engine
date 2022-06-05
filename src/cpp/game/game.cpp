#include "game.hpp"
#include "context.hpp"

#include "events/events.hpp"
#include "scripting/scripting.hpp"
#include "resources/resources.hpp"
#include "messages/messages.hpp"
#include "scheduler/scheduler.hpp"

#include <million/engine.hpp>

void game::setState (game::Context* context, entt::hashed_string new_state)
{
    EASY_BLOCK("game::setState", game::COLOR(1));
    auto new_state_name = new_state.data();
    spdlog::info("Setting game state to: {}", new_state_name);
    context->m_current_state = new_state;
    scripting::call(context->m_scripting_ctx, "set_game_state", new_state_name);
    auto it = context->m_game_scripts.find(new_state);
    if (it != context->m_game_scripts.end()) {
        context->m_current_game_script = it->second;
    } else {
        context->m_current_game_script = million::resources::Handle::invalid();
    }
}

void game::execute (game::Context* context, timing::Time current_time, timing::Delta delta, uint64_t current_frame)
{
    EASY_BLOCK("game::execute", game::COLOR(1));
    context->m_current_time = current_time;
    context->m_current_time_delta = delta;
    context->m_current_frame = current_frame;

    for (const auto& ev : events::events(context->m_events_ctx, "resources"_hs)) {
        EASY_BLOCK("Handling resource event", game::COLOR(3));
        switch (ev.type) {
            case events::resources::Loaded::ID:
            {
                auto& loaded = million::api::EngineRuntime::eventData<events::resources::Loaded>(ev);
                if (loaded.type == "game-script"_hs) {
                    context->m_game_scripts.emplace(loaded.name, loaded.handle);
                    if (scheduler::status(context->m_scheduler_ctx) == scheduler::SystemStatus::Loading && loaded.name == context->m_current_state) {
                        context->m_current_game_script = loaded.handle;
                        scheduler::setStatus(context->m_scheduler_ctx, scheduler::SystemStatus::Running);
                    }
                }
                break;
            }
            default:
                break;
        };
    }
}

void game::registerHandler (game::Context* context, entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler)
{
    context->m_game_handlers[state].push_back({events, handler});
}

void game::executeHandlers (game::Context* context)
{
    {
        EASY_BLOCK("Events/game", game::COLOR(1));
        auto& message_publisher = messages::publisher(context->m_messages_ctx);
        for (const auto& [event_stream, handler] : context->m_game_handlers[context->m_current_state]) {
            handler(events::events(context->m_events_ctx, event_stream), context->m_stream, message_publisher);
        }
    }
    if (context->m_current_game_script.valid()) {
        EASY_BLOCK("Scripts/game", game::COLOR(1));
        scripting::processGameEvents(context->m_scripting_ctx);
    }
}

timing::Delta game::deltaTime (game::Context* context)
{
    return context->m_current_time_delta;
}

timing::Time game::currentTime (game::Context* context)
{
    return context->m_current_time;
}

uint64_t game::currentFrame (game::Context* context)
{
    return context->m_current_frame;
}
