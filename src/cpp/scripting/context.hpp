#pragma once

#include <monkeys.hpp>
#include <lua.hpp>
#include <mutex>

struct BehaviorIterator {
    using const_iterable = typename entt::storage_traits<entt::entity, components::core::ScriptedBehavior>::storage_type::const_iterable;
    const_iterable::iterator next;
    const_iterable::iterator end;
};

namespace scripting {
    struct Context {
        lua_State* m_state;
        std::mutex m_vm_mutex;
        million::events::Stream* m_active_stream;
        phmap::flat_hash_map<entt::hashed_string::hash_type, entt::id_type, helpers::Identity> m_component_types;
        BehaviorIterator m_behavior_iterator;

        // Subsystem dependencies
        world::Context* m_world_ctx;
        messages::Context* m_messages_ctx;
        events::Context* m_events_ctx;
        resources::Context* m_resources_ctx;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::LightBlue900,
            profiler::colors::LightBlue700,
            profiler::colors::LightBlue500,
            profiler::colors::LightBlue300,
            profiler::colors::LightBlue100,
        };
        return colors[idx];
    }
}