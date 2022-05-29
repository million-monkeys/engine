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

        template <million::api::Module::CallbackMasks Which, typename Function>
        class Hook : public million::api::Module {
            using CM = million::api::Module::CallbackMasks;
        public:
            Hook(Function fn) : m_callback(fn) {}
            virtual ~Hook() {}

            std::uint32_t on_load (million::api::EngineSetup*) final { return 0; }
            void on_unload () final {}
            void on_before_reload () final {}
            void on_after_reload (million::api::EngineSetup*) final {}
            void on_game_setup (million::api::EngineSetup* api) final {
                call<CM::GAME_SETUP>(api);
            }
            void on_before_frame (million::api::EngineRuntime* api, timing::Time t, timing::Delta d, uint64_t f) final {
                call<CM::BEFORE_FRAME>(api, t, d, f);
            }
            void on_physics_step (timing::Delta d) final {
                call<CM::PHYSICS_STEP>(d);
            }
            void on_before_update (million::api::EngineRuntime* api) final {
                call<CM::BEFORE_UPDATE>(api);
            }
            void on_after_frame (million::api::EngineRuntime* api) final {
                call<CM::AFTER_FRAME>(api);
            }
            void on_prepare_render () final {
                call<CM::PREPARE_RENDER>();
            }
            void on_before_render () final {
                call<CM::BEFORE_RENDER>();
            }
            void on_after_render () final {
                call<CM::AFTER_RENDER>();
            }
            void on_load_scene (million::api::EngineSetup* api, entt::hashed_string::hash_type id, const std::string& n) final {
                call<CM::LOAD_SCENE>(api, id, n);
            }
            void on_unload_scene (entt::hashed_string::hash_type id, const std::string& n) final {
                call<CM::UNLOAD_SCENE>(id, n);
            }
        private:
            template <CM Expected, typename... Args>
            void call (Args... args) {
                if constexpr (Which == Expected) {
                    m_callback(args...);
                }
            }
            Function m_callback;
        };
    }

    template <million::api::Module::CallbackMasks Which, typename Function>
    million::api::Module* addHook (Context* context, Function callback)
    {
        auto hook = new hooks::Hook<Which, Function>(callback);
        addHook(context, Which, hook);
        return hook;
    }

}
