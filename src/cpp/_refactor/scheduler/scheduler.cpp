#include "scheduler.hpp"
#include "context.hpp"

#include "_refactor/world/world.hpp"
#include "_refactor/scripting/scripting.hpp"
#include "_refactor/physics/physics.hpp"
#include "_refactor/events/events.hpp"
#include "_refactor/game/game.hpp"
#include "_refactor/modules/modules.hpp"

using TaskCallback = void(*)(const void*, entt::registry&);

void scheduler::setStatus (scheduler::Context* context, scheduler::SystemStatus status)
{
    context->m_system_status = status;
}

scheduler::SystemStatus scheduler::status (scheduler::Context* context)
{
    return context->m_system_status;
}

void scheduler::execute (scheduler::Context* context)
{
    EASY_FUNCTION(scheduler::COLOR(1));
    if EXPECT_TAKEN(context->m_system_status == scheduler::SystemStatus::Running) {
        // Execute the taskflow graph if systems are running
        context->m_executor.run(context->m_coordinator);
        context->m_executor.wait_for_all();
    } else {
        // If systems are stopped, only pump events
        events::pump(context->m_events_ctx);
    }
}

entt::organizer& scheduler::organizer (scheduler::Context* context, million::SystemStage type)
{
    return context->m_organizers[type];
}

tf::Task createTask (scheduler::Context* context, tf::Taskflow* taskflow, const char* name, const void* userdata, TaskCallback callback)
{
    if (name) {
        auto fn = [context, userdata, callback, name](){
            SPDLOG_TRACE("Running System: {}", name);
            EASY_BLOCK(name, scheduler::COLOR(2));
            callback(userdata, world::registry(context->m_world_ctx));
        };
        return taskflow->emplace(fn).name(name);
    } else {
        auto fn = [context, userdata, callback](){
            EASY_BLOCK("Systems/task", scheduler::COLOR(2));
            callback(userdata, world::registry(context->m_world_ctx));
        };
        return taskflow->emplace(fn).name("Task");
    }
}

void scheduler::generateTasksForSystems (scheduler::Context* context)
{
    EASY_FUNCTION(scheduler::COLOR(2));
    SPDLOG_DEBUG("Generating task graph from systems");
    // Setup Systems by creating a Taskflow graph for each stage
    context->m_systems.clear();
    for (auto type : magic_enum::enum_values<million::SystemStage>()) {
        auto it = context->m_organizers.find(type);
        if (it != context->m_organizers.end()) {
            tf::Taskflow* taskflow;
            auto sit = context->m_systems.find(type);
            if (sit == context->m_systems.end()) {
                taskflow = new tf::Taskflow();
                context->m_systems[type] = taskflow;
            } else {
                taskflow = context->m_systems[type];
                taskflow->clear();
            }
            std::vector<std::pair<entt::organizer::vertex, tf::Task>> tasks;
            auto graph = it->second.graph();
            // First pass, prepare registry and create taskflow task
            for(auto&& node : graph) {
                auto name = node.name();
                if (name) {
                    SPDLOG_DEBUG("Setting up system: {}", name);
                }
                auto callback = node.callback();
                auto userdata = node.data();
                tasks.push_back({
                    node,
                    createTask(context, taskflow, name, userdata, callback)
                });
            }
            // Second pass, set parent-child relationship of tasks
            for (auto&& [node, task] : tasks) {
                for (auto index : node.children()) {
                    task.precede(tasks[index].second);
                }
            }
            it->second.clear();
        }
    }
    context->m_organizers.clear(); 
}

void scheduler::createTaskGraph (scheduler::Context* context)
{
    EASY_FUNCTION(scheduler::COLOR(1));
    SPDLOG_DEBUG("Generating core task graph");
    // Generate sub graph for systems
    generateTasksForSystems(context);

    /** Task graph:
     * 
     *                  SCRIPT EXECUTE                 AI EXECUTE [*]
     *                        |                              |
     *                  GAME LOGIC [*]              APPLY AI ACTIONS [*]
     *                    /         \                        |
     *           PHYSICS PREPARE    BEFORE UPDATE    AI SCRIPT EXECUTE
     *             |           \         |             /
     *      PHYSICS SIMULATE    +--> PUMP EVENTS <----+
     *                    \              |
     *                     +-> BEFORE UPDATE
     *                              |
     *                        UPDATE LOGIC [*]
     * // Copy current frames events for processing next frame
     * [*] = modules of subtasks
     **/
    SPDLOG_DEBUG("Creating task graph");

    Task events_game = context->m_coordinator.emplace([context](){
        SPDLOG_TRACE("Running Game event handlers");
        game::executeHandlers(context->m_game_ctx);
    }).name("events/game");

    Task events_scene = context->m_coordinator.emplace([context](){
        SPDLOG_TRACE("Running Scene event handlers");
        world::executeHandlers(context->m_world_ctx);
    }).name("events/scene");

    Task scripts_behavior = context->m_coordinator.emplace([context](){
        SPDLOG_TRACE("Running ScriptedBehaviors");
        scripting::processMessages(context->m_scripting_ctx);
    }).name("scripts/behavior");
    
    Task scripts_ai = context->m_coordinator.emplace([](){
        EASY_BLOCK("Scripts/AI", scheduler::COLOR(3));
        // execute lua scripts here
    }).name("scripts/ai");

    Task physics_prepare = context->m_coordinator.emplace([context](){
        EASY_BLOCK("Physics/prepare", scheduler::COLOR(3));
        auto& registry = world::registry(context->m_world_ctx);
        physics::prepare(context->m_physics_ctx, registry);
    }).name("phycis/prepare");

    Task physics_simulate = context->m_coordinator.emplace([context](){
        EASY_BLOCK("Physics/simulate", scheduler::COLOR(3));
        physics::simulate(context->m_physics_ctx);
    }).name("physics/simulate");

    Task pump_events = context->m_coordinator.emplace([context](){
        // Copy current frames events for processing next frame
        SPDLOG_TRACE("Pumping events");
        events::pump(context->m_events_ctx);
    }).name("events/pump");

    Task before_update = context->m_coordinator.emplace([context](){
        modules::hooks::before_update(context->m_modules_ctx);
    }).name("hooks/before-update");

    Task game_logic = context->m_coordinator.emplace([context](tf::Subflow& subflow){
        EASY_BLOCK("Systems/game-logic", scheduler::COLOR(3));
        tf::Taskflow* game_logic_flow = helpers::find_or(context->m_systems, million::SystemStage::GameLogic, nullptr);
        if EXPECT_TAKEN(game_logic_flow != nullptr) {
            subflow.composed_of(*game_logic_flow).name("systems/game-logic-subflow");
        }
    }).name("systems/game-logic");

    Task update_logic = context->m_coordinator.emplace([context](tf::Subflow& subflow){
        EASY_BLOCK("Systems/update", scheduler::COLOR(3));
        tf::Taskflow* updater_flow = helpers::find_or(context->m_systems, million::SystemStage::Update, nullptr);
        if EXPECT_TAKEN(updater_flow != nullptr) {
            subflow.composed_of(*updater_flow).name("systems/update-subflow");
        }
    }).name("systems/update");

    Task actions = context->m_coordinator.emplace([context](tf::Subflow& subflow){
        EASY_BLOCK("Systems/actions", scheduler::COLOR(3));
        tf::Taskflow* actions_flow = helpers::find_or(context->m_systems, million::SystemStage::Actions, nullptr);
        if EXPECT_TAKEN(actions_flow != nullptr) {
            subflow.composed_of(*actions_flow).name("systems/actions-subflow");
        }
    }).name("systems/actions");

    Task ai_execute = context->m_coordinator.emplace([context](tf::Subflow& subflow){
        EASY_BLOCK("AI/execute", scheduler::COLOR(3));
        tf::Taskflow* ai_execute_flow = helpers::find_or(context->m_systems, million::SystemStage::AIExecute, nullptr);
        if EXPECT_TAKEN(ai_execute_flow != nullptr) {
            subflow.composed_of(*ai_execute_flow).name("ai/execute-subflow");
        }
    }).name("ai/execute");

    // Game and scene event handlers
    events_scene.after(events_game);
    scripts_behavior.after(events_scene);

    before_update.after(pump_events);
    pump_events.after(scripts_behavior);

#ifdef DEBUG_BUILD
    const std::string& task_graph = entt::monostate<"dev/export-task-graph"_hs>();
    if (! task_graph.empty()) {
        std::ofstream file(task_graph, std::ios_base::out);
        context->m_coordinator.dump(file);
    }
#endif
}
