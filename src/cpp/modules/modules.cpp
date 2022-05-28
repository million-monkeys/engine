#include "modules.hpp"
#include "context.hpp"

#include <filesystem>

#ifdef DEBUG_BUILD
template <typename... Args>
void cr_debug_log (const std::string& fmt, Args... args) {
    std::string format = fmt;
    helpers::string_replace_inplace(format, "%s", "{}");
    helpers::string_replace_inplace(format, "%d", "{}");
    spdlog::debug(std::string{"[cr] "} + format, args...);
}
#define CR_DEBUG
#define CR_TRACE
#define CR_LOG(...)
#define CR_ERROR cr_debug_log
#endif
#define CR_HOST
#include <cr.h>

struct ModuleList {
    std::vector<cr_plugin> modules;
};

million::api::internal::ModuleManager* modules::api_module_manager (modules::Context* context)
{
    return context->m_module_manager;
}

million::api::EngineSetup* modules::api_engine_setup (modules::Context* context)
{
    return context->m_engine_setup;
}

million::api::EngineRuntime* modules::api_engine_runtime (modules::Context* context)
{
    return context->m_engine_runtime;
}

bool modules::load (modules::Context* context, ImGuiContext* imgui_context, const std::string& mods_path, const TomlValue& module_list)
{
    EASY_BLOCK("modules::load", modules::COLOR(1));

    if (context->m_modules == nullptr) {
        context->m_modules = new ModuleList;
    }

    auto api = modules::api_module_manager(context);

    bool success = true;
    for (const auto& module_config : module_list.as_array()) {
        if (toml::find_or<bool>(module_config, "enabled", false) && module_config.contains("path") && module_config.contains("name")) {
            bool required = toml::find_or<bool>(module_config, "required", false);

            auto info = new million::api::Module::Instance{
                toml::find<std::string>(module_config, "name"),
                context->logger,
                imgui_context,
                api,
                nullptr,
                required,
            };

            // Build file path and name
            std::filesystem::path path;
            if (mods_path.empty()) {
                path = std::filesystem::path{toml::find<std::string>(module_config, "path")};
            } else {
                path = std::filesystem::path{mods_path};
            }
            std::string filename = (path / info->name) .replace_extension(".module").string();

            // Load module
            spdlog::debug("[modules] Loading \"{}\" from: {}", info->name, filename);
            cr_plugin ctx;
            ctx.userdata = info;
            if (cr_plugin_open(ctx, filename.c_str())) {
                auto result = cr_plugin_update(ctx);
                if (result == 0) {
                    if (mods_path.empty()) {
                        spdlog::info("[modules] Loaded: {}", info->name);
                    } else {
                        spdlog::info("[modules] Loaded user module: {}", info->name);
                    }
                    context->m_modules->modules.push_back(ctx);
                    } else {
                        cr_plugin_close(ctx);
                        if (required) {
                            spdlog::error("[modules] Failed to load required module: {} ({})", info->name, result);
                            success = false;
                            break;
                        } else {
                            spdlog::warn("[modules] Failed to load module: {} ({})", info->name, result);
                        }
                    }
            } else {
                if (required) {
                    spdlog::error("[modules] Could not open required module \"{}\" from: {}", info->name, filename);
                    success = false;
                    break;
                } else {
                    spdlog::warn("[modules] Could not open module \"{}\" from: {}", info->name, filename);
                }
            }
        }
    }

    if (success) {
        for (auto& ctx : context->m_modules->modules) {
            auto info = static_cast<million::api::Module::Instance*>(ctx.userdata);
            EASY_BLOCK(info->name, modules::COLOR(2));
            auto flags = info->instance->on_load(context->m_engine_setup);
            if (flags == magic_enum::enum_integer(CM::MODULE_LOAD_ERROR)) {
                if (info->required) {
                    spdlog::error("[modules] Error while loading required module: {}", info->name);
                    return false;
                } else {
                    spdlog::warn("[modules] Error while loading module: {}", info->name);
                }
            } else {
                // Every module has a before_frame hook
                addHook(context, CM::BEFORE_FRAME, info->instance);
                // Register optional hooks
                for (auto& [hook, name] : magic_enum::enum_entries<CM>()) {
                    if (flags & magic_enum::enum_integer(hook)) {
                        SPDLOG_DEBUG("[modules] Adding hook \"{}\" to module: {}", name, info->name);
                        addHook(context, hook, info->instance);
                    }
                }
            }
        }
    } else {
        unload(context);
    }

    return success;
}

void modules::unload (modules::Context* context)
{
    EASY_BLOCK("modules::unload", modules::COLOR(1));

    // Remove all hooks
    context->m_hooks_beforeFrame.clear();
    context->m_hooks_afterFrame.clear();
    context->m_hooks_physicsStep.clear();
    context->m_hooks_beforeUpdate.clear();
    context->m_hooks_loadScene.clear();
    context->m_hooks_unloadScene.clear();
    context->m_hooks_prepareRender.clear();
    context->m_hooks_beforeRender.clear();
    context->m_hooks_afterRender.clear();

    // Unload each plugin in reverse load order
    for (auto& ctx : helpers::reverse(context->m_modules->modules)) {
        auto info = static_cast<million::api::Module::Instance*>(ctx.userdata);
        spdlog::info("[modules] Unloading: {}", info->name);
        cr_plugin_close(ctx);
        delete info;
    }

    // Make sure the list of modules is empty to prevent accidental use of deleted data
    context->m_modules->modules.clear();
}
