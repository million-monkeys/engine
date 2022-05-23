#include "world.hpp"
#include "context.hpp"

#include "_refactor/scripting/scripting.hpp"
#include "_refactor/events/events.hpp"
#include "_refactor/messages/messages.hpp"

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

// Swap foreground and background scenes, and clear the (new) background scene
void world::swapScenes (world::Context* context)
{
    EASY_FUNCTION(profiler::colors::Amber800);
    // Call UNLOAD_SCENE hooks on old scene
    // if (m_current.scene) {
    //     m_engine.callModuleHook<core::CM::UNLOAD_SCENE>(m_current_scene, m_scenes[m_current_scene]);
    // }

    // Set current scene and scripts
    context->m_current.scene = context->m_pending.scene;
    context->m_current.scripts = context->m_pending.scripts;

    // Invalidate pending
    context->m_pending.scene = 0;
    context->m_pending.scripts = million::resources::Handle::invalid();

    // Swap newly loaded scene into foreground
    context->m_registries.swap();
    // Copy entities marked as "global" from background to foreground
    context->m_registries.copyGlobals();
    // Clear the background registry
    context->m_registries.background().clear();
    // Set context variables
    context->m_registries.foreground().runtime.ctx().emplace<million::api::Runtime>(context->m_context_data);

    // Call LOAD_SCENE hooks on new scene
    // TODO: Create a scene API object to pass in? What can it do?
    if (context->m_current.scene) {
        // m_engine.callModuleHook<core::CM::LOAD_SCENE>(m_current_scene, m_scenes[m_current_scene]);
        context->m_stream.emit<events::scene::Activated>([context](auto& scene){
            scene.id = context->m_current.scene;
        });
    }
}

void world::processEvents (world::Context* context)
{
    EASY_FUNCTION(world::COLOR(2));
    if (context->m_current.scripts.valid()) {
        scripting::processSceneEvents(context->m_scripting_ctx, context->m_current.scripts);
    }
}

void world::executeHandlers (world::Context* context)
{
    EASY_FUNCTION(world::COLOR(1));
    {
        EASY_BLOCK("Events/scene", world::COLOR(2));
        auto& message_publisher = messages::publisher(context->m_messages_ctx);
        for (const auto& [event_stream, handler] : context->m_scene_handlers[context->m_current.scene]) {
            handler(events::events(context->m_events_ctx, event_stream), context->m_stream, message_publisher);
        }
    }
    world::processEvents(context);
}