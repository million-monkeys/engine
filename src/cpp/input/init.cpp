#include "input.hpp"
#include "context.hpp"

#include "events/events.hpp"

input::Context* input::init (events::Context* events_ctx)
{
    EASY_BLOCK("input::init", input::COLOR(1));
    SPDLOG_DEBUG("[input] Init");
    auto context = new input::Context{
        events::commandStream(events_ctx),
    };
    context->m_game_controller = nullptr;
    return context;
}

void input::term (input::Context* context)
{
    EASY_BLOCK("input::term", input::COLOR(1));
    SPDLOG_DEBUG("[input] Term");
    delete context;
}