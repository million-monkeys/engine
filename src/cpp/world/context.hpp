#pragma once

#include <monkeys.hpp>
#include "registries.hpp"

#include <filesystem>

struct PendingScene {
    phmap::flat_hash_set<million::resources::Handle::Type> resources;
    bool auto_swap;
};

struct SceneEventHandler {
    entt::hashed_string::hash_type events;
    million::GameHandler handler;
};

void loadComponent (world::Context* context, entt::registry& registry, entt::hashed_string component, entt::entity entity, const void* table);

namespace world {
    struct Context {
        Context (million::events::Stream& wrold_stream, million::events::Stream& scene_stream, million::events::Stream& commands) : m_world_stream(wrold_stream), m_scene_stream(scene_stream), m_blackboard_commands(commands) {}
        ~Context () {}

        events::Context* m_events_ctx;
        messages::Context* m_messages_ctx;
        resources::Context* m_resources_ctx;
        scripting::Context* m_scripting_ctx;
        modules::Context* m_modules_ctx;

        // Component loaders
        helpers::hashed_string_flat_map<million::api::definitions::LoaderFn> m_component_loaders;

        // ECS registries to manage all entities
        Registries m_registries;
        million::api::EngineRuntime* m_context_data = nullptr;

        million::events::Stream& m_world_stream;   // "world"_hs output stream, world subsystem events reported here
        million::events::Stream& m_scene_stream;   // "scene"_hs output stream, scene-specific user events reported here
        million::events::Stream& m_blackboard_commands; // "blackboard"_hs input stream
        
        // Scene loading
        std::filesystem::path m_path;
        helpers::hashed_string_flat_map<std::string> m_scenes;
        helpers::hashed_string_flat_map<PendingScene> m_pending_scenes;

        // Entity categories
        helpers::hashed_string_flat_map<std::uint16_t> m_category_bitfields;

        // Current scene and pending scene
        struct Info {
            entt::hashed_string::hash_type scene = 0;
            million::resources::Handle scripts;
        };
        Info m_current;
        Info m_pending;

        // Scene script event handlers
        helpers::hashed_string_flat_map<std::vector<SceneEventHandler>> m_scene_handlers;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Green900,
            profiler::colors::Green700,
            profiler::colors::Green500,
            profiler::colors::Green300,
            profiler::colors::Green100,
        };
        return colors[idx];
    }
}