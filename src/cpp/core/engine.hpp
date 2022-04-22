#pragma once

#include <game.hpp>
#include <taskflow/taskflow.hpp>
#include <million/definitions.hpp>
#include <entt/entity/organizer.hpp>
#include <world/scenes.hpp>

#include "scripting/core.hpp"

#include <SDL.h>

namespace core {
    using CM = monkeys::api::Module::CallbackMasks;

    // Component used by the prototypes registry to identify the prototype entity
    struct EntityPrototypeID {
        entt::hashed_string::hash_type id;
    };

    class Engine : public monkeys::api::Engine
    {
    public:
        using EventPool = memory::heterogeneous::StackPool<memory::alignment::AlignCacheLine>;

        Engine();
        virtual ~Engine();

        // Public API
        void readBinaryFile (const std::string& filename, std::string& buffer) const final;
        entt::registry& registry (monkeys::Registry) final;
        entt::organizer& organizer (monkeys::SystemStage) final;
        entt::entity findEntity (entt::hashed_string) const final;
        const std::string& findEntityName (const components::core::Named&) const final;
        entt::entity loadEntity (monkeys::Registry, entt::hashed_string) final;
        void mergeEntity (monkeys::Registry, entt::entity, entt::hashed_string, bool) final;
        monkeys::resources::Handle findResource (entt::hashed_string::hash_type) final;
        const monkeys::events::Iterable& events () final;

        // Load component and add it to entity
        void loadComponent (entt::registry& registry, entt::hashed_string, entt::entity, const void*);

        // Time
        DeltaTime deltaTime () { return m_current_time_delta; }

        bool init ();
        void reset ();

        // Execute the Taskflow graph of tasks, returns true if still running
        bool execute (Time current_time, DeltaTime delta, uint64_t frame_count);

        // Register module hooks
        void registerModule (std::uint32_t, monkeys::api::Module*);

        // Call all modules that are added as a specific engine hook
        template <monkeys::api::Module::CallbackMasks Hook, typename... T> void callModuleHook (T... args) {
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

        // Allocate space for an event in the event stream, returning a raw pointer to its payload
        std::byte* allocateEvent (entt::hashed_string::hash_type, entt::entity, std::uint8_t) final;

        // std::byte* allocateEventInternal (entt::hashed_string id, entt::entity target, std::uint8_t size)
        // {
        //     auto envelope = m_event_pool.emplace<monkeys::events::Envelope>(T::EventID, target, size);
        //     return m_event_pool.unaligned_allocate(size);
        // }

    private:
        void* allocModule(std::size_t) final;
        void deallocModule(void *) final;
        void installComponent (const monkeys::api::definitions::Component& component) final;

        // Add a module to be called by a specific engine hook
        void addModuleHook (monkeys::api::Module::CallbackMasks hook, monkeys::api::Module* module);

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

        // Make previously emitted events visible to consumers
        void pumpEvents ();
        void refreshEventsIterable ();

        // Emit an event directly to the global pool (warning: unsynchronised)
        template <typename T>
        [[maybe_unused]] T& internalEmitEvent (entt::entity target=entt::null)
        {
            auto envelope = m_event_pool.emplace<monkeys::events::Envelope>(T::EventID, target, sizeof(T));
            m_event_pool.unaligned_allocate(sizeof(T));
            return *(new (reinterpret_cast<std::byte*>(envelope) + sizeof(monkeys::events::Envelope)) T{});
        }
        [[maybe_unused]] void internalEmitEvent (entt::hashed_string id, entt::entity target=entt::null)
        {
            m_event_pool.emplace<monkeys::events::Envelope>(id, target, std::uint32_t(0));
        }

        struct NamedEntityInfo {
            entt::entity entity;
            std::string name;
        };

        // ECS registries to manage all entities
        entt::registry m_runtime_registry;
        entt::registry m_background_registry;
        entt::registry m_prototype_registry;

        phmap::flat_hash_map<entt::hashed_string::hash_type, monkeys::api::definitions::LoaderFn, helpers::Identity> m_component_loaders;
        phmap::flat_hash_map<entt::hashed_string::hash_type, NamedEntityInfo, helpers::Identity> m_named_entities;
        phmap::flat_hash_map<entt::hashed_string::hash_type, entt::entity, helpers::Identity> m_prototype_entities;
        world::SceneManager m_scene_manager;
        const std::string m_empty_string = {};

        // System and task scheduling
        phmap::flat_hash_map<monkeys::SystemStage, entt::organizer> m_organizers;
        tf::Taskflow m_coordinator;
        tf::Executor m_executor;

        enum class SystemStatus {
            Running,
            Stopped,
        };
        SystemStatus m_system_status;

        // Timing
        DeltaTime m_current_time_delta = 0;

        // Event system
        monkeys::events::Iterable m_events_iterable;
        std::vector<EventPool*> m_event_pools;
        EventPool m_event_pool;

        // Module Hooks
        std::vector<monkeys::api::Module*> m_hooks_beforeFrame;
        std::vector<monkeys::api::Module*> m_hooks_afterFrame;
        std::vector<monkeys::api::Module*> m_hooks_beforeUpdate;
        std::vector<monkeys::api::Module*> m_hooks_loadScene;
        std::vector<monkeys::api::Module*> m_hooks_unloadScene;
        std::vector<monkeys::api::Module*> m_hooks_prepareRender;
        std::vector<monkeys::api::Module*> m_hooks_beforeRender;
        std::vector<monkeys::api::Module*> m_hooks_afterRender;

        // User input
        SDL_GameController* m_game_controller;
    };

}
