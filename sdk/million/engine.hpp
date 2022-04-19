#pragma once

#include <entt/fwd.hpp>
#include <magic_enum.hpp>

#include "types.hpp"
#include "definitions.hpp"

namespace monkeys {
    // Access to the ECS registry
    enum class Registry : std::uint32_t {
        // The main registry, used to run the game
        Runtime,
        // The background registry, used for background loading, scene editing etc, can be copied to the Runtime registry
        Background,
        // The prototype registry, used by the component loader setup code, not meant for module users
        Prototype,
    };

    enum class SystemStage {
        GameLogic,
        AIExecute,
        AIActions,
        Update,
    };
}

namespace monkeys::api {

    // The engine-provided API to modules
    class Engine
    {
    public:
        virtual ~Engine() {}

        /* Internal API */
        /* ------------ */

        /** Used internally by module boilerplate to ensure ECS type ID's are consistent across modules */
        // virtual detail::type_context *type_context() const = 0;

        /** Internal module creation API */
        template <class Module>
        Module* createModule (const std::string& name)
        {
            return new (allocModule(sizeof(Module))) Module(name, *this);
        }

        /** Internal module destruction API */
        template <class Module>
        void destroyModule (Module* mod)
        {
            mod->~Module();
            deallocModule(mod);
        }

        /** Internal component registration template */
        template <typename T>
        void registerComponent (const monkeys::api::definitions::Component& component)
        {
            magic_enum::enum_for_each<monkeys::Registry>([this] (auto which) {
                static_cast<void>(registry(which).template storage<T>());
            });
            installComponent(component);
        }

        /* User API (Note that users should use the API wrapper classes in gou.hpp) */
        /* ------------------------------------------------------------------------ */

        virtual void readBinaryFile (const std::string& filename, std::string& buffer) const = 0;

        /** Access events emitted last frame */
        // virtual const detail::EventsIterator& events() = 0;

        /** Access an ECS registry */
        virtual entt::registry& registry (monkeys::Registry) = 0;

        /** Access ECS organizers through which to register systems */
        virtual entt::organizer& organizer (monkeys::SystemStage) = 0;

        /** Find a named entity. Returns entt::null if no such entity exists */
        virtual entt::entity findEntity (entt::hashed_string) const = 0;

        /** Get the string name of a named entity */
        virtual const std::string& findEntityName (const components::core::Named& named) const = 0;

        /** Load an entity into specified registry from a template */
        virtual entt::entity loadEntity (monkeys::Registry, entt::hashed_string) = 0;

        /** Merge a template into an entity in the specified registrywhich */
        virtual void mergeEntity (monkeys::Registry, entt::entity, entt::hashed_string, bool) = 0;

        // Retrieve a resource handle by name
        virtual monkeys::resources::Handle findResource (entt::hashed_string::hash_type) = 0;

    private:
        // Allow engine to decide where the module classes are allocated
        virtual void* allocModule(std::size_t) = 0;
        virtual void deallocModule(void *) = 0;
        // Register components with the engine
        virtual void installComponent (const monkeys::api::definitions::Component& component) = 0;
    };
}
