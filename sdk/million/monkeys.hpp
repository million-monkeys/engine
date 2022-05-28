#ifndef MILLION_million__HPP
#define MILLION_million__HPP

#include <components/core.hpp>
#include <events/engine.hpp>

#include "types.hpp"
#include "module.hpp"
#include "engine.hpp"

#include <entt/entity/registry.hpp>
#include <entt/entity/organizer.hpp>

#include "macros.hpp"

namespace million {
    using Setup = million::api::EngineSetup;
    using Runtime = million::api::EngineRuntime;
}

namespace mm_module {
    namespace detail {
        HAS_MEMBER_FUNCTION  (onLoad,          (std::declval<million::Setup*>()))
        HAS_MEMBER_FUNCTION  (onGameSetup,     (std::declval<million::Setup*>()))
        HAS_MEMBER_FUNCTION  (onUnload,        ())
        HAS_MEMBER_FUNCTION  (onBeforeReload,  ())
        HAS_MEMBER_FUNCTION  (onAfterReload,   (std::declval<million::Setup*>()))
        HAS_MEMBER_FUNCTION  (onBeforeUpdate,  (std::declval<million::Runtime*>()))
        HAS_MEMBER_FUNCTION  (onPhysicsStep,   (std::declval<timing::Delta>()))
        HAS_MEMBER_FUNCTION  (onAfterFrame,    (std::declval<million::Runtime*>()))
        HAS_MEMBER_FUNCTION  (onPrepareRender, ())
        HAS_MEMBER_FUNCTION  (onBeforeRender,  ())
        HAS_MEMBER_FUNCTION  (onAfterRender,   ())
        HAS_MEMBER_FUNCTION  (onLoadScene,     (std::declval<million::Setup*>(), std::declval<entt::hashed_string::hash_type>(), std::declval<const std::string&>()))
        HAS_MEMBER_FUNCTION  (onUnloadScene,   (std::declval<entt::hashed_string::hash_type>(), std::declval<const std::string&>()))
        HAS_MEMBER_FUNCTION_N(onBeforeFrame,1, (std::declval<million::Runtime*>(), std::declval<timing::Time>(), std::declval<timing::Delta>(), std::declval<uint64_t>()))
        HAS_MEMBER_FUNCTION_N(onBeforeFrame,2, (std::declval<million::Runtime*>(), std::declval<timing::Time>(), std::declval<timing::Delta>(), std::declval<uint64_t>()))
        HAS_MEMBER_FUNCTION_N(onBeforeFrame,3, (std::declval<million::Runtime*>(), std::declval<timing::Time>(), std::declval<timing::Delta>(), std::declval<uint64_t>()))
        HAS_MEMBER_FUNCTION_N(onBeforeFrame,4, (std::declval<million::Runtime*>(), std::declval<timing::Time>(), std::declval<timing::Delta>(), std::declval<uint64_t>()))
    }

    template <typename Derived>
    class Module : public million::api::Module {
    public:
        Module (const std::string& name) : m_moduleName{name} {}
        virtual ~Module () {}
        /*
        * Logging functions
        */
        template <typename... Args> void info  (const std::string& text, Args... args)  {spdlog::info(m_moduleName + text, args...);}
        template <typename... Args> void warn  (const std::string& text, Args... args)  {spdlog::warn(m_moduleName + text, args...);}
        template <typename... Args> void error (const std::string& text, Args... args)  {spdlog::error(m_moduleName + text, args...);}
        template <typename... Args> void debug (const std::string& text, Args... args)  {
    #ifdef DEBUG_BUILD
            spdlog::debug(m_moduleName + text, args...);
    #endif
        }

///////////////////////////////////////////////////////////////////////////////
// Implementation Details, not part of public API
///////////////////////////////////////////////////////////////////////////////

    private:
        std::uint32_t on_load (million::Setup* api) final {
            using CM = million::api::Module::CallbackMasks;
            if constexpr (detail::hasMember_onLoad<Derived>()) {
                if (! static_cast<Derived*>(this)->onLoad(api)) {
                    return static_cast<std::underlying_type_t<CM>>(CM::MODULE_LOAD_ERROR);
                }
            }

            uint32_t flags = 0;
            if constexpr (detail::hasMember_onGameSetup<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::GAME_SETUP);
            } if constexpr (detail::hasMember_onPhysicsStep<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::PHYSICS_STEP);
            } if constexpr (detail::hasMember_onBeforeFrame1<Derived>() || detail::hasMember_onBeforeFrame2<Derived>() || detail::hasMember_onBeforeFrame3<Derived>() || detail::hasMember_onBeforeFrame4<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::BEFORE_FRAME);
            } if constexpr (detail::hasMember_onBeforeUpdate<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::BEFORE_UPDATE);
            } if constexpr (detail::hasMember_onAfterFrame<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::AFTER_FRAME);
            } if constexpr (detail::hasMember_onPrepareRender<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::PREPARE_RENDER);
            }
            if constexpr (detail::hasMember_onBeforeRender<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::BEFORE_RENDER);
            }
            if constexpr (detail::hasMember_onAfterRender<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::AFTER_RENDER);
            }
            if constexpr (detail::hasMember_onLoadScene<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::LOAD_SCENE);
            }
            if constexpr (detail::hasMember_onUnloadScene<Derived>()) {
                flags |= static_cast<std::underlying_type_t<CM>>(CM::UNLOAD_SCENE);
            }
            return flags;
        }

        void on_game_setup (million::Setup* api) final {
            if constexpr (detail::hasMember_onGameSetup<Derived>()) {
                static_cast<Derived*>(this)->onGameSetup(api);
            }
        }

        void on_unload () final {
            if constexpr (detail::hasMember_onUnload<Derived>()) {
                static_cast<Derived*>(this)->onUnload();
            }
        }

        void on_before_reload () final {
            if constexpr (detail::hasMember_onBeforeReload<Derived>()) {
                static_cast<Derived*>(this)->onBeforeReload();
            }
        }

        void on_after_reload (million::Setup* api) final {
            if constexpr (detail::hasMember_onAfterReload<Derived>()) {
                static_cast<Derived*>(this)->onAfterReload(api);
            }
        }

        void on_before_frame (million::Runtime* api, timing::Time time, timing::Delta delta, uint64_t frame) final {
            if constexpr (detail::hasMember_onBeforeFrame1<Derived>()) {
                static_cast<Derived*>(this)->onBeforeFrame(api, time, delta, frame);
            }
            if constexpr (detail::hasMember_onBeforeFrame2<Derived>()) {
                static_cast<Derived*>(this)->onBeforeFrame(api, time, delta);
            }
            if constexpr (detail::hasMember_onBeforeFrame3<Derived>()) {
                static_cast<Derived*>(this)->onBeforeFrame(api, time);
            }
            if constexpr (detail::hasMember_onBeforeFrame4<Derived>()) {
                static_cast<Derived*>(this)->onBeforeFrame(api);
            }
        }

        void on_physics_step (timing::Delta time_step) final {
            if constexpr (detail::hasMember_onPhysicsStep<Derived>()) {
                static_cast<Derived*>(this)->onPhysicsStep(time_step);
            }
        }

        void on_before_update (million::Runtime* api) final {
            if constexpr (detail::hasMember_onBeforeUpdate<Derived>()) {
                static_cast<Derived*>(this)->onBeforeUpdate(api);
            }
        }

        void on_after_frame (million::Runtime* api) final {
            if constexpr (detail::hasMember_onAfterFrame<Derived>()) {
                static_cast<Derived*>(this)->onAfterFrame(api);
            }
        }

        void on_prepare_render () final {
            if constexpr (detail::hasMember_onPrepareRender<Derived>()) {
                static_cast<Derived*>(this)->onPrepareRender();
            }
        }

        void on_before_render () final {
            if constexpr (detail::hasMember_onBeforeRender<Derived>()) {
                static_cast<Derived*>(this)->onBeforeRender();
            }
        }

        void on_after_render () final {
            if constexpr (detail::hasMember_onAfterRender<Derived>()) {
                static_cast<Derived*>(this)->onAfterRender();
            }
        }

        void on_load_scene (million::Setup* api, entt::hashed_string::hash_type scene_id, const std::string& scene_name) final {
            if constexpr (detail::hasMember_onLoadScene<Derived>()) {
                static_cast<Derived*>(this)->onLoadScene(api, scene_id, scene_name);
            }
        }

        void on_unload_scene (entt::hashed_string::hash_type scene_id, const std::string& scene_name) final {
            if constexpr (detail::hasMember_onUnloadScene<Derived>()) {
                static_cast<Derived*>(this)->onUnloadScene(scene_id, scene_name);
            }
        }

        const std::string m_moduleName;
    };

    #define MM_MODULE_INIT(NAMESPACE) million::api::Module* NAMESPACE init (const std::string& name, million::api::internal::ModuleManager* mm)
    MM_MODULE_INIT();
#ifndef NO_COMPONENTS
    void register_components(million::api::internal::ModuleManager*);
    #define MM_REGISTER_COMPONENTS mm_module::register_components(mm);
#else
    #define MM_REGISTER_COMPONENTS
#endif
}

#endif