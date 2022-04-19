#pragma once

#include <game.hpp>
#include <taskflow/taskflow.hpp>
#include <million/definitions.hpp>
#include <entt/entity/organizer.hpp>

#include <world/scenes.hpp>

namespace core {
    // Component used by the prototypes registry to identify the prototype entity
    struct EntityPrototypeID {
        entt::hashed_string::hash_type id;
    };

    class Engine : public monkeys::api::Engine
    {
    public:
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

        // Load component and add it to entity
        void loadComponent (entt::registry& registry, entt::hashed_string, entt::entity, const void*);

        bool init ();
        void reset ();

    private:
        void* allocModule(std::size_t) final;
        void deallocModule(void *) final;
        void installComponent (const monkeys::api::definitions::Component& component) final;

        // Copy all entities from one registry to another
        void copyRegistry (const entt::registry& from, entt::registry& to);

        // Callbacks to manage Named entities
        void onAddNamedEntity (entt::registry&, entt::entity);
        void onRemoveNamedEntity (entt::registry&, entt::entity);

        // Callbacks to manage prototype entities
        void onAddPrototypeEntity (entt::registry&, entt::entity);
        void onRemovePrototypeEntity (entt::registry&, entt::entity);

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
    };

}
