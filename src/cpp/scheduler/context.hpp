#pragma once

#include <monkeys.hpp>
#include <taskflow/taskflow.hpp>
#include <entt/entity/organizer.hpp>

#include "scheduler.hpp"

#include <atomic>

class Task {
public:
    Task (tf::Task t) : task(t) {}
    ~Task() {}

    template <typename... Args> void after (Args... args) {task.succeed((args.task)...);}
    template <typename... Args> void before (Args... args) {task.precede((args.task)...);}

    Task& operator>> (Task& other) {task.precede(other.task); return other;}

    tf::Task task;
};

class WorkerDecorator : public tf::WorkerInterface {
public:
    void scheduler_prologue(tf::Worker& w) override;
    void scheduler_epilogue(tf::Worker& w, std::exception_ptr e) override;
};

namespace scheduler {
    struct Context {
        Context (int num_workers) : m_executor(num_workers, std::make_shared<WorkerDecorator>()) {}
        ~Context () {}

        world::Context* m_world_ctx;
        scripting::Context* m_scripting_ctx;
        events::Context* m_events_ctx;
        game::Context* m_game_ctx;
        modules::Context* m_modules_ctx;

        million::api::Module* m_module;

        phmap::flat_hash_map<million::SystemStage, entt::organizer> m_organizers;
        tf::Taskflow m_coordinator;
        tf::Executor m_executor;
        phmap::flat_hash_map<million::SystemStage, tf::Taskflow*> m_systems;

        SystemStatus m_system_status;

        float m_timestep_cccumulator;
        float m_step_size;
        unsigned m_frames_late;

        std::atomic_bool m_ok = true;
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
