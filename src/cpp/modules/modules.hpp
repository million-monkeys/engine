#pragma once

#include <monkeys.hpp>
#include "utils/parser.hpp"

struct ImGuiContext;

namespace modules {
    Context* init (std::shared_ptr<spdlog::logger> logger);
    void term (Context* context);

    void setup_api (Context* context, world::Context* world_ctx, game::Context* game_ctx, resources::Context* resources_ctx, scheduler::Context* scheduler_ctx, messages::Context* messages_ctx, events::Context* events_ctx);
    million::api::internal::ModuleManager* api_module_manager (Context* context);
    million::api::EngineSetup* api_engine_setup (Context* context);
    million::api::EngineRuntime* api_engine_runtime (Context* context);

    bool load (Context* context, ImGuiContext* imgui_context, const std::string& base_path, const TomlValue& module_list);
    void unload (Context* context);

    void addHook (Context* context, million::api::Module::CallbackMasks hook, million::api::Module* mod);

    namespace hooks {
        void game_setup (Context* context);
        void before_frame (Context* context, timing::Time, timing::Delta, uint64_t);
        void physics_step (Context* context, timing::Delta);
        void before_update (Context* context);
        void after_frame (Context* context);
        void prepare_render (Context* context);
        void before_render (Context* context);
        void after_render (Context* context);
        void load_scene (Context* context, entt::hashed_string::hash_type scene_id, const std::string& name);
        void unload_scene (Context* context, entt::hashed_string::hash_type scene_id, const std::string& name);
    }
}
