#include "modules.hpp"
#include "context.hpp"

million::api::internal::ModuleManager* modules::api_module_manager (modules::Context* context)
{
    return context->m_module_manager;
}

million::api::EngineSetup* modules::api_engine_setup (modules::Context* context)
{
    return context->m_engine_setup;
}