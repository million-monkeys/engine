#pragma once

#include <game.hpp>
#include <taskflow/taskflow.hpp>
#include <million/definitions.hpp>
#include <entt/entity/organizer.hpp>
#include <world/scenes.hpp>
#include "scheduler.hpp"

#include "event_pools.hpp"

namespace core {
    using CM = million::api::Module::CallbackMasks;

    struct StreamInfo {
        IterableStream* iterable;
        million::events::Stream* streamable;
    };

    // Component used by the prototypes registry to identify the prototype entity
    struct EntityPrototypeID {
        entt::hashed_string::hash_type id;
    };

    enum class HandlerType {
        Game,
        Scene,
    };

    class Engine : public million::api::internal::ModuleManager, public million::api::EngineSetup, public million::api::EngineRuntime
    {
    public:
        Engine(helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes);
        virtual ~Engine();

        /////////////////////////////////////////
        // Public API
        /////////////////////////////////////////

        // Setup API
        void registerGameHandler (entt::hashed_string state, million::GameHandler handler) final;
        void registerSceneHandler (entt::hashed_string scene, million::SceneHandler handler) final;
        void readBinaryFile (const std::string& filename, std::string& buffer) const final;
        million::resources::Handle loadResource (entt::hashed_string, const std::string&, entt::hashed_string::hash_type) final;
        entt::registry& registry (million::Registry) final;
        entt::organizer& organizer (million::SystemStage) final;
        million::events::Publisher& publisher() final;
        million::events::Stream& commandStream() final;
        million::events::Stream& createStream (entt::hashed_string, million::StreamWriters=million::StreamWriters::Single, std::uint32_t=0) final;

        // Runtime API
        entt::entity findEntity (entt::hashed_string) const final;
        const std::string& findEntityName (const components::core::Named&) const final;
        entt::entity loadEntity (million::Registry, entt::hashed_string) final;
        void mergeEntity (million::Registry, entt::entity, entt::hashed_string, bool) final;
        million::resources::Handle findResource (entt::hashed_string::hash_type) final;
        const million::events::MessageIterable messages () const final;
        const million::events::EventIterable events (entt::hashed_string) const final;

        /////////////////////////////////////////
        // Internal API
        /////////////////////////////////////////

        // Load component and add it to entity
        void loadComponent (entt::registry& registry, entt::hashed_string, entt::entity, const void*);

        // Time
        DeltaTime deltaTime () { return m_current_time_delta; }

        bool init ();
        void shutdown ();

        void setupGame ();

        // Execute the Taskflow graph of tasks, returns true if still running
        bool execute (Time current_time, DeltaTime delta, uint64_t frame_count);

        void executeHandlers (HandlerType type);

        // Register module hooks
        void registerModule (std::uint32_t, million::api::Module*);

        // Call all modules that are added as a specific engine hook
        template <million::api::Module::CallbackMasks Hook, typename... T> void callModuleHook (T... args) {
            if constexpr (Hook == CM::BEFORE_FRAME) {
                EASY_BLOCK("callModuleHook<BEFORE_FRAME>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_beforeFrame) {
                    mod->on_before_frame(args...);
                }
            } else if constexpr (Hook == CM::AFTER_FRAME) {
                EASY_BLOCK("callModuleHook<AFTER_FRAME>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_afterFrame) {
                    mod->on_after_frame(args...);
                }
            } else if constexpr (Hook == CM::LOAD_SCENE) {
                EASY_BLOCK("callModuleHook<LOAD_SCENE>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_loadScene) {
                    mod->on_load_scene(args...);
                }
            } else if constexpr (Hook == CM::UNLOAD_SCENE) {
                EASY_BLOCK("callModuleHook<UNLOAD_SCENE>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_unloadScene) {
                    mod->on_unload_scene(args...);
                }
            } else if constexpr (Hook == CM::PREPARE_RENDER) {
                EASY_BLOCK("callModuleHook<PREPARE_RENDER>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_prepareRender) {
                    mod->on_prepare_render(args...);
                }
            } else if constexpr (Hook == CM::BEFORE_RENDER) {
                EASY_BLOCK("callModuleHook<BEFORE_RENDER>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_beforeRender) {
                    mod->on_before_render(args...);
                }
            } else if constexpr (Hook == CM::AFTER_RENDER) {
                EASY_BLOCK("callModuleHook<AFTER_RENDER>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_afterRender) {
                    mod->on_after_render(args...);
                }
            } else if constexpr (Hook == CM::BEFORE_UPDATE) {
                EASY_BLOCK("callModuleHook<BEFORE_UPDATE>", profiler::colors::Indigo100);
                for (auto& mod : m_hooks_beforeUpdate) {
                    mod->on_before_update(args...);
                }
            }
        }

    private:
        void* allocModule(std::size_t) final;
        void deallocModule(void *) final;
        void installComponent (const million::api::definitions::Component& component) final;

        // Add a module to be called by a specific engine hook
        void addModuleHook (million::api::Module::CallbackMasks hook, million::api::Module* module);

        // Copy all entities from one registry to another
        void copyRegistry (const entt::registry& from, entt::registry& to);

        // Callbacks to manage Named entities
        void onAddNamedEntity (entt::registry&, entt::entity);
        void onRemoveNamedEntity (entt::registry&, entt::entity);

        // Callbacks to manage prototype entities
        void onAddPrototypeEntity (entt::registry&, entt::entity);
        void onRemovePrototypeEntity (entt::registry&, entt::entity);

        // Process input events
        void handleInput ();

        // Make previously emitted script events visible to consumers
        void pumpScriptEvents ();
        // Make previously emitted events visible to consumers
        void pumpEvents ();

        void setGameState (entt::hashed_string new_state);

        // Emit an event directly to the global pool (warning: unsynchronised)
        template <typename T>
        [[maybe_unused]] T& internalEmitEvent (entt::entity target=entt::null)
        {
            // auto envelope = m_event_pool.emplace<million::events::Envelope>(T::EventID, target, sizeof(T));
            // m_event_pool.unaligned_allocate(sizeof(T));
            // return *(new (reinterpret_cast<std::byte*>(envelope) + sizeof(million::events::Envelope)) T{});
        }
        [[maybe_unused]] void internalEmitEvent (entt::hashed_string id, entt::entity target=entt::null)
        {
            // m_event_pool.push(id, target, std::uint32_t(0));
        }

        struct NamedEntityInfo {
            entt::entity entity;
            std::string name;
        };

        // ECS registries to manage all entities
        entt::registry m_runtime_registry;
        entt::registry m_background_registry;
        entt::registry m_prototype_registry;

        helpers::hashed_string_flat_map<million::api::definitions::LoaderFn> m_component_loaders;
        helpers::hashed_string_flat_map<NamedEntityInfo> m_named_entities;
        helpers::hashed_string_flat_map<entt::entity> m_prototype_entities;
        world::SceneManager m_scene_manager;
        const std::string m_empty_string = {};

        // System and task scheduling
        scheduler::Scheduler m_scheduler;

        enum class SystemStatus {
            Running,
            Stopped,
        };
        SystemStatus m_system_status;                           // Track whether systems should be run or not
        entt::hashed_string::hash_type m_current_game_state;    // Track whether on menu, loading screen, etc

        helpers::hashed_string_flat_map<std::vector<million::GameHandler>> m_game_handlers;
        helpers::hashed_string_flat_map<std::vector<million::SceneHandler>> m_scene_handlers;

        // Timing
        DeltaTime m_current_time_delta = 0;

        // Event system
        helpers::hashed_string_flat_map<std::uint32_t>& m_stream_sizes;
        std::vector<EventPool> m_event_pools;
        EventPoolBase::PoolType m_event_pool;
        helpers::hashed_string_node_map<StreamInfo> m_named_streams;
        million::events::Stream& m_commands;

        // Module Hooks
        std::vector<million::api::Module*> m_hooks_beforeFrame;
        std::vector<million::api::Module*> m_hooks_afterFrame;
        std::vector<million::api::Module*> m_hooks_beforeUpdate;
        std::vector<million::api::Module*> m_hooks_loadScene;
        std::vector<million::api::Module*> m_hooks_unloadScene;
        std::vector<million::api::Module*> m_hooks_prepareRender;
        std::vector<million::api::Module*> m_hooks_beforeRender;
        std::vector<million::api::Module*> m_hooks_afterRender;

        // User input
        struct InputData* m_input_data;

        // Resource name bindings
        helpers::hashed_string_flat_map<million::resources::Handle> m_named_resources;

        friend class scheduler::Scheduler;
    };

}
