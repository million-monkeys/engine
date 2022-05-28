#include "modules.hpp"
#include "context.hpp"

void modules::addHook (modules::Context* context, million::api::Module::CallbackMasks hook, million::api::Module* mod)
{
    EASY_FUNCTION(modules::COLOR(3));
    switch (hook) {
        case CM::GAME_SETUP:
            context->m_hooks_gameSetup.push_back(mod);
            break;
        case CM::BEFORE_FRAME:
            context->m_hooks_beforeFrame.push_back(mod);
            break;
        case CM::AFTER_FRAME:
            context->m_hooks_afterFrame.push_back(mod);
            break;
        case CM::PHYSICS_STEP:
            context->m_hooks_physicsStep.push_back(mod);
            break;
        case CM::BEFORE_UPDATE:
            context->m_hooks_beforeUpdate.push_back(mod);
            break;
        case CM::PREPARE_RENDER:
            context->m_hooks_prepareRender.push_back(mod);
            break;
        case CM::BEFORE_RENDER:
            context->m_hooks_beforeRender.push_back(mod);
            break;
        case CM::AFTER_RENDER:
            context->m_hooks_afterRender.push_back(mod);
            break;
        case CM::LOAD_SCENE:
            context->m_hooks_loadScene.push_back(mod);
            break;
        case CM::UNLOAD_SCENE:
            context->m_hooks_unloadScene.push_back(mod);
            break;
        case CM::MODULE_LOAD_ERROR:
            break;
    };
}

void modules::hooks::game_setup (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_gameSetup) {
        mod->on_game_setup(context->m_engine_setup);
    }
}

void modules::hooks::before_frame (modules::Context* context, timing::Time time, timing::Delta delta, uint64_t frame)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_beforeFrame) {
        mod->on_before_frame(context->m_engine_runtime, time, delta, frame);
    }
}

void modules::hooks::physics_step (modules::Context* context, timing::Delta time_delta)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_physicsStep) {
        mod->on_physics_step(time_delta);
    }
}

void modules::hooks::before_update (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_beforeUpdate) {
        mod->on_before_update(context->m_engine_runtime);
    }
}

void modules::hooks::after_frame (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_afterFrame) {
        mod->on_after_frame(context->m_engine_runtime);
    }
}

void modules::hooks::prepare_render (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_prepareRender) {
        mod->on_prepare_render();
    }
}

void modules::hooks::before_render (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_beforeRender) {
        mod->on_before_render();
    }
}

void modules::hooks::after_render (modules::Context* context)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_afterRender) {
        mod->on_after_render();
    }
}

void modules::hooks::load_scene (modules::Context* context, entt::hashed_string::hash_type scene_id, const std::string& name)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_loadScene) {
        mod->on_load_scene(context->m_engine_setup, scene_id, name);
    }
}

void modules::hooks::unload_scene (modules::Context* context, entt::hashed_string::hash_type scene_id, const std::string& name)
{
    EASY_FUNCTION(modules::COLOR(1));
    for (auto& mod : context->m_hooks_unloadScene) {
        mod->on_unload_scene(scene_id, name);
    }
}