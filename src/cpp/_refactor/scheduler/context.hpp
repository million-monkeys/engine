#pragma once

#include <monkeys.hpp>
#include <taskflow/taskflow.hpp>
#include <entt/entity/organizer.hpp>

#include "scheduler.hpp"

class Task {
public:
    Task (tf::Task t) : task(t) {}
    ~Task() {}

    template <typename... Args> void after (Args... args) {task.succeed((args.task)...);}
    template <typename... Args> void before (Args... args) {task.precede((args.task)...);}

    tf::Task task;
};


namespace scheduler {
    struct Context {
        world::Context* m_world_ctx;
        scripting::Context* m_scripting_ctx;
        physics::Context* m_physics_ctx;
        events::Context* m_events_ctx;
        game::Context* m_game_ctx;
        modules::Context* m_modules_ctx;

        phmap::flat_hash_map<million::SystemStage, entt::organizer> m_organizers;
        tf::Taskflow m_coordinator;
        tf::Executor m_executor;
        phmap::flat_hash_map<million::SystemStage, tf::Taskflow*> m_systems;

        SystemStatus m_system_status;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Yellow900,
            profiler::colors::Yellow700,
            profiler::colors::Yellow500,
            profiler::colors::Yellow300,
            profiler::colors::Yellow100,
        };
        return colors[idx];
    }
}
