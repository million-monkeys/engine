#include "game.hpp"
#include "context.hpp"

#include "_refactor/events/events.hpp"

game::Context* game::init (events::Context* events_ctx, messages::Context* messages_ctx, world::Context* world_ctx, scripting::Context* scripting_ctx, resources::Context* resources_ctx)
{
    auto context = new game::Context{
        events::createStream(events_ctx, "game"_hs),
    };
    context->m_events_ctx = events_ctx;
    context->m_messages_ctx = messages_ctx;
    context->m_world_ctx = world_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_resources_ctx = resources_ctx;
    return context;
}

void game::term (game::Context* context)
{
    delete context;
}
