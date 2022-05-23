#include "input.hpp"
#include "context.hpp"

#include "_refactor/events/events.hpp"

input::Context* input::init (events::Context* events_ctx)
{
    auto context = new input::Context{
        events::commandStream(events_ctx),
    };
    context->m_game_controller = nullptr;
    return context;
}

void input::term (input::Context* context)
{
    delete context;
}