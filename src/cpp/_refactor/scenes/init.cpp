#include "scenes.hpp"
#include "context.hpp"

#include "_refactor/events/events.hpp"

scenes::Context* scenes::init (events::Context* events_ctx, resources::Context* resources_ctx, scripting::Context* scripting_ctx)
{
    auto context = new scenes::Context{events::createStream(events_ctx, "scenes"_hs)};
    context->m_events_ctx = events_ctx;
    context->m_resources_ctx = resources_ctx;
    context->m_scripting_ctx = scripting_ctx;
    return context;
}

void scenes::term (scenes::Context* context)
{
    delete context;
}

void scenes::setContextData (scenes::Context* context, million::api::EngineRuntime* runtime)
{
    context->m_context_data = runtime;
}