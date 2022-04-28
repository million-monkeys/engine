#pragma once

#include <game.hpp>
#include <taskflow/taskflow.hpp>
#include <entt/entity/organizer.hpp>

namespace physics {
    struct Context;
}

namespace core {
    class Engine;
}

namespace scheduler {

    class Scheduler {
    public:
        void createTaskGraph (core::Engine& engine);
        void execute ();

        entt::organizer& organizer(million::SystemStage type) { return m_organizers[type]; }

    private:
        // Task System
        phmap::flat_hash_map<million::SystemStage, entt::organizer> m_organizers;
        tf::Taskflow m_coordinator;
        tf::Executor m_executor;

        // Services
        [[maybe_unused]] physics::Context* m_physics_context;
    };

}
