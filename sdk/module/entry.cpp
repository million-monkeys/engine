
#include <million/module.hpp>
#include <million/engine.hpp>

#include <cr.h>
#include <entt/core/type_info.hpp>
#include <spdlog/spdlog.h>

// #include <imgui.h>

namespace mm_module {
    million::api::Module* init (const std::string& name, million::api::internal::ModuleManager* api);
}

/********************************************************************************
 * Plugin entrypoint and setup
 ********************************************************************************/

million::api::Module* million_module = nullptr;
million::api::EngineSetup* engine_setup = nullptr;

CR_EXPORT int cr_main(cr_plugin* ctx, cr_op operation)
{
    if(million_module == nullptr) {
        auto info = static_cast<million::api::Module::Instance*>(ctx->userdata);
        // Setup EnTT component type ID generator
        // gou::ctx::ref = info->engine->type_context();
        // Set spdlog logger
        spdlog::set_default_logger(info->logger);
        // Set Dear ImGUI context
        // ImGui::SetCurrentContext(info->imgui_context);
        // Create instance of module class
        million_module = mm_module::init(
            std::string{"["} + info->name + std::string{"] "},
            info->api
        );
        // Register module with engine
        info->instance = million_module;
    }
    switch (operation) {
        // Hot-code reloading
        case CR_LOAD:
        {
            million_module->on_after_reload(engine_setup);
            break;
        }
        case CR_UNLOAD:
        {
            million_module->on_before_reload();
            break;
        }
        // Update step
        case CR_STEP:
        {
            break;
        }
        // Close and unload module
        case CR_CLOSE:
        {
            million_module->on_unload();
            break;
        }
    }
    return 0;
}
