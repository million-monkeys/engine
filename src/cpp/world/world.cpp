#include "world.hpp"
#include "context.hpp"

#include "scripting/scripting.hpp"
#include "events/events.hpp"
#include "messages/messages.hpp"

void world::update (world::Context* context)
{
    if (! context->m_pending_scenes.empty()) {
        auto iter = events::events(context->m_events_ctx, "resources"_hs);
        if (iter.size() > 0) {
            EASY_BLOCK("SceneManager handling resource events", profiler::colors::Amber300);
            for (const auto& ev : iter) {   
                switch (ev.type) {
                    case events::resources::Loaded::ID:
                    {
                        auto& loaded = million::api::EngineRuntime::eventData<events::resources::Loaded>(ev);
                        auto it = context->m_pending_scenes.find(loaded.name);
                        if (it != context->m_pending_scenes.end()) {
                            EASY_BLOCK("SceneManager handling loaded event", profiler::colors::Amber500);
                            PendingScene& pending = it->second;
                            SPDLOG_TRACE("[world] Updating pending scenes {} {}", pending.resources.size(), loaded.handle.handle);
                            pending.resources.erase(loaded.handle.handle);
                            if (loaded.type == "scene-script"_hs) {
                                context->m_pending.scripts = loaded.handle;
                            }
                            if (pending.resources.empty()) {
                                // Scene fully loaded
                                context->m_pending.scene = loaded.name;
                                context->m_stream.emit<events::scene::Loaded>([&loaded](auto& scene){
                                    scene.id = loaded.name;
                                });
                                if (pending.auto_swap) {
                                    world::swapScenes(context);
                                }
                                context->m_pending_scenes.erase(it);
                            } else {
                                spdlog::debug("[world] Waiting for scene to load: {} resources pending", pending.resources.size());
                            }
                        }
                        break;
                    }
                    default:
                        break;
                };
            }
        }
    }
}

void world::processEvents (world::Context* context)
{
    EASY_FUNCTION(world::COLOR(2));
    if (context->m_current.scripts.valid()) {
        scripting::processSceneEvents(context->m_scripting_ctx, context->m_current.scripts);
    } else {
        SPDLOG_TRACE("[world] No scene event scripts");
    }
}

void world::registerHandler (world::Context* context, entt::hashed_string scene, entt::hashed_string::hash_type events, million::SceneHandler handler)
{
    context->m_scene_handlers[scene].push_back({events, handler});
}

void world::executeHandlers (world::Context* context)
{
    EASY_FUNCTION(world::COLOR(1));
    SPDLOG_TRACE("[world] Executing handlers");
    {
        EASY_BLOCK("Events/scene", world::COLOR(2));
        auto& message_publisher = messages::publisher(context->m_messages_ctx);
        for (const auto& [event_stream, handler] : context->m_scene_handlers[context->m_current.scene]) {
            handler(events::events(context->m_events_ctx, event_stream), context->m_stream, message_publisher);
        }
    }
    world::processEvents(context);
}
