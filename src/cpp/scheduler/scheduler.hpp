#pragma once

#include <monkeys.hpp>

namespace scheduler {
    enum class SystemStatus {
        Running,
        Loading,
        Stopped,
    };

    Context* init (world::Context* world_ctx, scripting::Context* scripting_ctx, events::Context* events_ctx, game::Context* game_ctx, modules::Context* modules_ctx);
    void term (Context* context);

    void setStatus (Context* context, SystemStatus status);
    SystemStatus status (Context* context);

    void generateTasksForSystems (Context* context);
    void createTaskGraph (Context* context);
    bool execute (Context* context);
    entt::organizer& organizer(Context* context, million::SystemStage type);
}