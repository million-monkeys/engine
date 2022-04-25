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

    namespace resources {
        class Loader {
        public:
            virtual ~Loader() {}
            virtual bool load (monkeys::resources::Handle handle, const std::string& filename) = 0;
            virtual void unload (monkeys::resources::Handle handle) = 0;

            virtual entt::hashed_string name () const = 0;
        };
    }

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
        template <class Module> Module* createModule (const std::string& name)
        {
            return new (allocModule(sizeof(Module))) Module(name, *this);
        }

        /** Internal module destruction API */
        template <class Module> void destroyModule (Module* mod)
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

        // Allocate space for an event in the event stream, returning a raw pointer to its payload
        virtual std::byte* allocateEvent (entt::hashed_string::hash_type, entt::entity, std::uint8_t) = 0;

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

        /** Load an entity into specified registry from a prototype */
        virtual entt::entity loadEntity (monkeys::Registry, entt::hashed_string) = 0;

        /** Merge a prototype into an entity in the specified registry which */
        virtual void mergeEntity (monkeys::Registry, entt::entity, entt::hashed_string, bool) = 0;

        // Retrieve a resource handle by name
        virtual monkeys::resources::Handle findResource (entt::hashed_string::hash_type) = 0;

        // Load a new resource
        virtual monkeys::resources::Handle loadResource (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name) = 0;

        monkeys::resources::Handle loadResource (entt::hashed_string type, const std::string& filename)
        {
            return loadResource(type, filename, 0);
        }

        /** Emit an event to the event stream */
        template <typename T>
        [[maybe_unused]] T& emit (entt::entity target=entt::null)
        {
            return *(new (allocateEvent(T::EventID, target, sizeof(T))) T{});
        }

        /** Access an iterable collection of all emitted events */
        virtual const monkeys::events::Iterable& events () = 0;

        /** Retrieve the payload from an individual event */
        template <typename EventT>
        const EventT& eventData (const monkeys::events::Envelope& envelope) {
            if (EventT::EventID == envelope.type && sizeof(EventT) == envelope.size) {
                return *reinterpret_cast<const EventT*>(reinterpret_cast<const std::byte*>(&envelope) + sizeof(monkeys::events::Envelope));
            } else {
                spdlog::error("Could not cast event {} to {}", envelope.type, EventT::EventID.data());
                throw std::runtime_error("bad event type");
            }
        }

    private:
        // Allow engine to decide where the module classes are allocated
        virtual void* allocModule(std::size_t) = 0;
        virtual void deallocModule(void *) = 0;
        // Register components with the engine
        virtual void installComponent (const monkeys::api::definitions::Component& component) = 0;
    };
}
