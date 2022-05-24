#include "world.hpp"
#include "context.hpp"

#include "loaders/scene_entities.hpp"
#include "loaders/scene_scripts.hpp"
#include "loaders/entity_scripts.hpp"

#include "events/events.hpp"
#include "resources/resources.hpp"

world::Context* world::init (events::Context* events_ctx, messages::Context* messages_ctx, resources::Context* resources_ctx, scripting::Context* scripting_ctx, modules::Context* modules_ctx)
{
    auto context = new world::Context{
        events::createStream(events_ctx, "scenes"_hs),
        events::createStream(events_ctx, "blackboard"_hs, million::StreamWriters::Multi),
    };
    context->m_events_ctx = events_ctx;
    context->m_messages_ctx = messages_ctx;
    context->m_resources_ctx = resources_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_modules_ctx = modules_ctx;

    resources::install<loaders::SceneScripts>(resources_ctx, context);
    resources::install<loaders::SceneEntities>(resources_ctx, context);
    resources::install<loaders::EntityScripts>(resources_ctx, context);

    return context;
}

void world::term (world::Context* context)
{
    delete context;
}

void world::setContextData (world::Context* context, million::api::EngineRuntime* runtime)
{
    context->m_context_data = runtime;
}
