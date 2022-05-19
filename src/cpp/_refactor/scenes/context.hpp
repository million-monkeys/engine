#pragma once

#include <game.hpp>
#include "registries.hpp"

#include <filesystem>

struct PendingScene {
    phmap::flat_hash_set<million::resources::Handle::Type> resources;
    bool auto_swap;
};

// Component used to represent group membership. Component named storage is used for different groups.
struct EntityGroup {};

namespace scenes {
    struct Context {
        Context (million::events::Stream& stream) : m_stream(stream) {}
        ~Context () {}

        events::Context* m_events_ctx;
        resources::Context* m_resources_ctx;
        scripting::Context* m_scripting_ctx;

        // ECS registries to manage all entities
        Registries m_registries;
        million::api::EngineRuntime* m_context_data = nullptr;

        million::events::Stream& m_stream;
        std::filesystem::path m_path;
        helpers::hashed_string_flat_map<std::string> m_scenes;
        helpers::hashed_string_flat_map<PendingScene> m_pending_scenes;
        struct Info {
            entt::hashed_string::hash_type scene = 0;
            million::resources::Handle scripts;
        };
        Info m_current;
        Info m_pending;
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