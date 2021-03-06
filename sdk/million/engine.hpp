#pragma once

#include <entt/fwd.hpp>

#include "types.hpp"
#include "definitions.hpp"
#include "game_systems.hpp"

#include <spdlog/spdlog.h>

namespace components::core {
    struct Named;
}

namespace million {
    enum class SystemStage {
        GameLogic,
        AIExecute,
        Actions,
        Update,
    };

    enum class StreamWriters {
        Single,
        Multi,
    };

    using GameHandler = void (*)(const million::events::EventIterable events, million::events::Stream& stream, million::events::Publisher& publisher);
    using SceneHandler = void (*)(const million::events::EventIterable events, million::events::Stream& stream, million::events::Publisher& publisher);
}

// The engine-provided API to modules
namespace million::api {

    namespace resources {
        class Loader {
        public:
            virtual ~Loader() {}
            virtual bool cached (const std::string& filename, std::uint32_t*) = 0; // MUST be threadsafe

            virtual bool load (million::resources::Handle handle, const std::string& filename) = 0;
            virtual void unload (million::resources::Handle handle) = 0;

            virtual entt::hashed_string name () const = 0;
        };
    }

    /* Internal API (Used internally to setup modules, should not be used directly)             */
    /* ---------------------------------------------------------------------------------------- */
    namespace internal {
        // Create and manage modules.
        class ModuleManager
        {
        public:
            virtual ~ModuleManager() {}
            /** Used internally by module boilerplate to ensure ECS type ID's are consistent across modules */
            // virtual detail::type_context *type_context() const = 0;

            /** Internal module creation API */
            template <class Module> Module* createModule (const std::string& name)
            {
                return new (allocModule(sizeof(Module))) Module(name);
            }

            /** Internal module destruction API */
            template <class Module> void destroyModule (Module* mod)
            {
                mod->~Module();
                deallocModule(mod);
            }

            /** Internal component registration template */
            template <typename T>
            void registerComponent (const million::api::definitions::Component& component)
            {
                installComponent(component, [](entt::registry& registry) {
                    static_cast<void>(registry.template storage<T>());
                });
            }

            template <typename... Ts>
            void registerInternalComponents ()
            {
                if constexpr (sizeof...(Ts) > 0) {
                    prepareRegistries([](entt::registry& registry) {
                        (static_cast<void>(registry.template storage<Ts>()), ...);
                    });
                }
            }
            
        protected:
            // Allow engine to decide where the module classes are allocated
            virtual void* allocModule(std::size_t) = 0;
            virtual void deallocModule(void *) = 0;
            // Register components with the engine
            virtual void installComponent (const million::api::definitions::Component& component, million::api::definitions::PrepareFn prepareFn) = 0;
            // Add unloadable (internal) components to the engine
            virtual void prepareRegistries (million::api::definitions::PrepareFn prepareFn) = 0;
        };
    }

    /* User API (Note that users should use the API wrapper classes instead of this directly)   */
    /* ---------------------------------------------------------------------------------------- */

    // Engine API used at setup (ie on startup or when loading a new scene).
    // NOT thread safe, must not be called concurrently with itself or with EngineRuntime.
    class EngineSetup
    {
    public:
        virtual ~EngineSetup() {}

        /** Register an event handler to be run for a specific game state */
        virtual void registerGameHandler (entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler) = 0;

        /** Register an event handler to be run for a specific scene */
        virtual void registerSceneHandler (entt::hashed_string scene, entt::hashed_string::hash_type events, million::SceneHandler handler) = 0;

        virtual void registerResourceLoader (million::api::resources::Loader* loader) = 0;

        virtual void readBinaryFile (const std::string& filename, std::string& buffer) const = 0;

        // Retrieve a resource handle by name
        virtual million::resources::Handle findResource (entt::hashed_string::hash_type) const = 0;

        // Load a new resource
        virtual million::resources::Handle loadResource (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name) = 0;

        /** Access ECS registry */
        virtual entt::registry& registry () = 0;

        /** Access ECS organizers through which to register systems */
        virtual entt::organizer& organizer (million::SystemStage) = 0;

        /** Get the global message publisher. This publisher should not be passed to another thread, instead each thread should get its own reference using this function */
        virtual million::events::Publisher& publisher() = 0;

        /** Get the global engine commands event stream, used to control the engine */
        virtual million::events::Stream& commandStream() = 0;

        /** Create a new named event stream */
        virtual million::events::Stream& createStream (entt::hashed_string, million::StreamWriters=million::StreamWriters::Single) = 0;
    };

    // Engine API to be used at runtime (ie in systems or handler each frame).
    // Thread safe, can be called any time at runtime but must NOT be called concurrently with EngineSetup.
    class EngineRuntime
    {
    public:
        virtual ~EngineRuntime() {}
        /** Find a named entity. Returns entt::null if no such entity exists */
        virtual entt::entity findEntity (entt::hashed_string) const = 0;

        /** Get the string name of a named entity */
        virtual const std::string& findEntityName (const components::core::Named& named) const = 0;
        virtual const std::string& findEntityName (entt::entity entity) const = 0;

        /** Load an entity into specified registry from a prototype */
        // NOT Thread Safe!
        // TODO: make thread safe by making asynchronous and dispatching event when done.
        virtual entt::entity loadEntity (entt::hashed_string) = 0;

        /** Merge a prototype into an entity in the specified registry which */
        // NOT Thread Safe!
        // TODO: make thread safe by making asynchronous and dispatching event when done.
        virtual void mergeEntity (entt::entity, entt::hashed_string, bool) = 0;

        // Retrieve a resource handle by name
        virtual million::resources::Handle findResource (entt::hashed_string::hash_type) const = 0;

        // Load a new resource
        virtual million::resources::Handle loadResource (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name) = 0;

        million::resources::Handle loadResource (entt::hashed_string type, const std::string& filename)
        {
            return loadResource(type, filename, 0);
        }

        /** Get the global message publisher. This publisher should not be passed to another thread, instead each thread should get its own reference using this function */
        virtual million::events::Publisher& publisher() = 0;

        /** Get the global engine commands event stream, used to control the engine */
        virtual million::events::Stream& commandStream() = 0;

        /** Retrieve events from a named event stream  */
        virtual const million::events::EventIterable events (entt::hashed_string) const = 0;

        /** Retrieve the payload from an individual event */
        template <typename EventT, typename Envelope>
        static const EventT& eventData (const Envelope& envelope) {
            if (EventT::ID == envelope.type && sizeof(EventT) == envelope.size) {
                return *reinterpret_cast<const EventT*>(reinterpret_cast<const std::byte*>(&envelope) + sizeof(Envelope));
            } else {
                spdlog::error("Could not cast event {} to {}", envelope.type, EventT::ID.data());
                throw std::runtime_error("bad event type");
            }
        }
    };

    // Wrapper to provide runtime API to systems through organizer context variables. Must be const to avoid systems from running serially
    class Runtime {
    public:
        Runtime() {}
        Runtime(EngineRuntime* runtime) : m_runtime(runtime) {}
        Runtime(Runtime&& other) : m_runtime(other.m_runtime) {}
        ~Runtime() {}

        entt::entity findEntity (entt::hashed_string name) const
        {
            return m_runtime->findEntity(name);
        }

        /** Get the string name of a named entity */
        const std::string& findEntityName (const components::core::Named& named) const
        {
            return m_runtime->findEntityName(named);
        }

        const std::string& findEntityName (entt::entity entity) const
        {
            return m_runtime->findEntityName(entity);
        }

        // Retrieve a resource handle by name
        million::resources::Handle findResource (entt::hashed_string::hash_type name) const
        {
            return m_runtime->findResource(name);
        }

        /** Get the global message publisher. This publisher should not be passed to another thread, instead each thread should get its own reference using this function */
        million::events::Publisher& publisher() const
        {
            return m_runtime->publisher();
        }

        /** Get the global engine commands event stream, used to control the engine */
        million::events::Stream& commandStream() const
        {
            return m_runtime->commandStream();
        }

        /** Retrieve events from a named event stream  */
        const million::events::EventIterable events (entt::hashed_string stream_name) const
        {
            return m_runtime->events(stream_name);
        }

        /** Retrieve the payload from an individual event */
        template <typename EventT, typename Envelope>
        const EventT& eventData (const Envelope& envelope) const {
            return m_runtime->eventData<EventT, Envelope>(envelope);
        }
        
    private:
        mutable EngineRuntime* m_runtime = nullptr;
    };
}
