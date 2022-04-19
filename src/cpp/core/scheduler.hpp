#pragma once

#include <game.hpp>
#include <taskflow/taskflow.hpp>
#include <entt/entity/organizer.hpp>

namespace physics {
    struct Context;
}

namespace scheduler {

    class Scheduler {
    public:
        void createTaskGraph ();

    private:
        // Entity System
        entt::registry m_registry;

        // Task System
        phmap::flat_hash_map<monkeys::SystemStage, entt::organizer> m_organizers;
        tf::Taskflow m_coordinator;
        tf::Executor m_executor;

        // Services
        [[maybe_unused]] physics::Context* m_physics_context;
    };

}
