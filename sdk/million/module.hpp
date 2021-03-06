#pragma once

#include "types.hpp"
#include <string>
#include <spdlog/spdlog.h>

struct ImGuiContext;

namespace million::api {
    namespace internal {
        class ModuleManager;
    }

    class EngineSetup;
    class EngineRuntime;

    // The module-provided API to the engine
    class Module {
    public:
        virtual ~Module() {}
        enum class CallbackMasks : std::uint32_t {
            // load, unload and before_frame are always called. reload functions are handled by the module (not the engine)
            BEFORE_FRAME        = 0x00, // Doesn't need to set a bit because its always registered for every module
            PHYSICS_STEP        = 0x01,
            BEFORE_UPDATE       = 0x02,
            AFTER_FRAME         = 0x04,
            PREPARE_RENDER      = 0x08,
            BEFORE_RENDER       = 0x10,
            AFTER_RENDER        = 0x20,
            LOAD_SCENE          = 0x40,
            UNLOAD_SCENE        = 0x80,
            GAME_SETUP          = 0x100,
            MODULE_LOAD_ERROR   = 0xFFFFFFFF,

            /** Notes on data access.
             * It is always safe to access data passed into the hooks as arguments. Sharing data between hooks, however, must follow some rules.
             * The Million million game engine executes hooks in one of two thread contexts: 'engine' and 'renderer'
             * You must not share data (eg member variables of your Module instance) between these two contexts, as they may run concurrently, unless
             * the data is atomic or protected by a lock, however locks should be avoided to maintain high performance.
             * 
             * Hooks that execute in the 'engine' context are:
             *  onLoad, onUnload, onBeforeReload, onAfterReload, onBeforeFrame, onBeforeUpdate, onAfterFrame, onLoadScene, onUnloadScene and onPrepareRender
             * 
             * Hooks that execute in the 'renderer' context are:
             *  onBeforeRender, onAfterRender and onPrepareRender
             * 
             * It is safe to access both engine and render data from onPrepareRender (technically it runs in the 'renderer' context, but inside a critical
             * section, allowing safe access to the engine -- however, logic in onPrepareRender should be kept to a minimum).
             * 
             * While systems execute in the 'engine' context, it is not, however, safe to access the engine from inside a system as they may be scheduled in parallel.
             * Instead, systems should communicate either through their writeable components on the entities they process, or by setting local member data in
             * their Module, that you know isn't being accessed by another system and then using a hook (eg onBeforeUpdate or onAfterFrame) to communicate the
             * data elsewhere.
             * 
             * It is safe to emit events at any time from the 'engine' context, even in systems. The 'render' context (excluding onPrepareRender) should not
             * emit events.
             */
        };

        // Module lifecycle. Use these to setup and shutdown your module, setting up global (non-scene-specific) systems.
        virtual std::uint32_t on_load (EngineSetup*) = 0;
        virtual void on_game_setup (EngineSetup*) = 0;
        virtual void on_unload () = 0;
        // Dev-mode hot-code reload lifecycle functions.
        virtual void on_before_reload () = 0; // Before hot code reload, use to persist data
        virtual void on_after_reload (EngineSetup*) = 0; // After hot code reload, use to reload data
        // Logic hooks. Use these to add custom logic on a per-frame basis.
        virtual void on_before_frame (EngineRuntime*, timing::Time, timing::Delta, uint64_t) = 0;
        virtual void on_physics_step (timing::Delta) = 0;
        virtual void on_before_update (EngineRuntime*) = 0;
        virtual void on_after_frame (EngineRuntime*) = 0;
        // Rendering hooks. Use these to add custom rendering, including dev tool UI.
        virtual void on_prepare_render () = 0;
        virtual void on_before_render () = 0;
        virtual void on_after_render () = 0;
        // Scene setup and teardown. Use these to set up scene logic and scene-specific systems.
        virtual void on_load_scene (EngineSetup*, entt::hashed_string::hash_type, const std::string&) = 0;
        virtual void on_unload_scene (entt::hashed_string::hash_type, const std::string&) = 0;

        // Information passed to an instance
        struct Instance {
            const std::string name;
            const std::shared_ptr<spdlog::logger> logger;
            ImGuiContext* imgui_context;
            million::api::internal::ModuleManager* api;
            Module* instance;
            bool required;
        };
    };
}