#include "modules.hpp"
#include "context.hpp"

#include "api_module_manager.hpp"
#include "api_setup.hpp"
#include "api_runtime.hpp"

modules::Context* modules::init (std::shared_ptr<spdlog::logger> logger)
{
    EASY_BLOCK("modules::init", modules::COLOR(1));
    SPDLOG_DEBUG("[modules] Init");
    auto context = new modules::Context{};
    context->logger = logger;
    return context;
}

void modules::setup_api (modules::Context* context, world::Context* world_ctx, game::Context* game_ctx, resources::Context* resources_ctx, scheduler::Context* scheduler_ctx, messages::Context* messages_ctx, events::Context* events_ctx)
{
    context->m_module_manager = new ModuleManagerAPI(world_ctx);
    context->m_engine_setup = new SetupAPI(world_ctx, game_ctx, resources_ctx, scheduler_ctx, messages_ctx, events_ctx);
    context->m_engine_runtime = new RuntimeAPI(world_ctx, resources_ctx, messages_ctx, events_ctx);
}

void modules::term (modules::Context* context)
{
    EASY_BLOCK("modules::term", modules::COLOR(1));
    SPDLOG_DEBUG("[modules] Term");
    delete context->m_engine_setup;
    delete context->m_module_manager;
    delete context;
}
