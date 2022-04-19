
#include "scheduler.hpp"

using namespace scheduler;

namespace physics {
    // Gather physics data from registry
    void prepare (struct Context*, const entt::registry&)
    {

    }

    // Step the physics simulation
    void simulate (struct Context*)
    {

    }
}

void pumpEvents ()
{

}

void Scheduler::createTaskGraph ()
{
#if 0
    // Setup Systems by creating a Taskflow graph for each stage
    using Stage = SystemStage;
    auto registry = &m_registry;
    phmap::flat_hash_map<SystemStage, tf::Taskflow*> taskflows;
    for (auto type : magic_enum::enum_values<SystemStage>()) {
        auto it = m_organizers.find(type);
        if (it != m_organizers.end()) {
            tf::Taskflow* taskflow = new tf::Taskflow();
            taskflows[type] = taskflow;
            std::vector<std::pair<entt::organizer::vertex, tf::Task>> tasks;
            auto graph = it->second.graph();
            // First pass, prepare registry and create taskflow task
            for(auto&& node : graph) {
                SPDLOG_DEBUG("Setting up system: {}", node.name());
                node.prepare(m_registry);
                auto callback = node.callback();
                auto userdata = node.data();
                auto name = node.name();
                tasks.push_back({
                    node,
                    taskflow->emplace([callback, userdata, registry, name](){
                        SPDLOG_TRACE("Running System: {}", name);
                        callback(userdata, *registry);
                    }).name(node.name())
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
    // Any new additions will never be added to the Taskflow graph, so no point in keeping the organizers around
    m_organizers.clear();

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
     * 
     * [*] = modules of subtasks
     **/
    tf::Task script_execute_task = m_coordinator.emplace([this](){
        EASY_BLOCK("Game/scripts", profiler::colors::Purple100);
        // execute lua scripts here
    }).name("game/scripts");
    tf::Task script_ai_task = m_coordinator.emplace([this](){
        EASY_BLOCK("AI/scripts", profiler::colors::Purple100);
        // execute lua scripts here
    }).name("ai/scripts");
    tf::Task script_execute_task = m_coordinator.emplace([this](){
        EASY_BLOCK("Scripts/execute", profiler::colors::Purple100);
        physics::prepare(m_physics_context, m_registry);
    }).name("scripts/execute");    
    tf::Task physics_prepare_task = m_coordinator.emplace([this](){
        EASY_BLOCK("Physics/prepare", profiler::colors::Purple100);
        physics::prepare(m_physics_context, m_registry);
    }).name("physics/prepare");
    tf::Task physics_simulate_task = m_coordinator.emplace([this](){
        EASY_BLOCK("Physics/simulate", profiler::colors::Purple200);
        physics::simulate(m_physics_context);
    }).name("physics/simulate");
    tf::Task before_update_task = m_coordinator.emplace([this](){
        // call before_update hooks here
        // callModuleHook<CM::BEFORE_UPDATE>();
    }).name("hooks/before-update");
    tf::Task pump_events_task = m_coordinator.emplace([this](){
        pumpEvents(); // Copy current frames events for processing next frame
    }).name("events/pump");
    pump_events_task.succeed(before_update_task, physics_prepare_task, script_ai_task);
    physics_simulate_task.succeed(physics_prepare_task);
    
    // Add engine-internal tasks to graph and coordinate flow into one graph
    if (tf::Taskflow* game_logic_flow = helpers::find_or(taskflows, Stage::GameLogic, nullptr)) {
        game_logic_flow->name("Game Logic");
        tf::Task game_logic_tasks = m_coordinator.composed_of(*game_logic_flow).name("game/systems");
#ifdef BUILD_WITH_EASY_PROFILER
        tf::Task profiler_before_logic_task = m_coordinator.emplace([](){
            EASY_NONSCOPED_BLOCK("Game/Systems", profiler::colors::Indigo400);
        }).name("profiler/before-game-systems");
        tf::Task profiler_after_logic_task = m_coordinator.emplace([](){
            EASY_END_BLOCK;
        }).name("profiler/after-game-systems");
        script_execute_task.precede(profiler_before_logic_task);
        game_logic_tasks.succeed(profiler_before_logic_task);
        profiler_after_logic_task.succeed(game_logic_tasks);
        profiler_after_logic_task.precede(before_update_task, physics_prepare_task);
#else
        script_execute_task.precede(game_logic_tasks);
        game_logic_tasks.precede(before_update_task, physics_prepare_task);
#endif
    } else {
        script_execute_task.precede(physics_prepare_task, before_update_task);
    }

    if (tf::Taskflow* updater_flow = helpers::find_or(taskflows, Stage::Update, nullptr)) {
        updater_flow->name("State Update");
        tf::Task updater_tasks = m_coordinator.composed_of(*updater_flow).name("systems/update");
#ifdef BUILD_WITH_EASY_PROFILER
        tf::Task profiler_before_update_task = m_coordinator.emplace([](){
            EASY_NONSCOPED_BLOCK("Systems/update", profiler::colors::Indigo600);
        }).name("profiler/before-systems-update");
        tf::Task profiler_after_updates_task = m_coordinator.emplace([](){
            EASY_END_BLOCK;
        }).name("profiler/after-systems-update");
        before_update_task.precede(profiler_before_update_task);
        updater_tasks.succeed(profiler_before_update_task);
        profiler_after_updates_task.succeed(updater_tasks);
#else
        updater_tasks.succeed(before_update_task);
#endif
    }

    std::optional<tf::Task> maybe_ai_actions_task;
    if (tf::Taskflow* ai_actions_flow = helpers::find_or(taskflows, Stage::AIActions, nullptr)) {
        ai_actions_flow->name("Apply AI Actions");
        tf::Task ai_actions_task = m_coordinator.composed_of(*ai_actions_flow).name("ai/actions");
#ifdef BUILD_WITH_EASY_PROFILER
        tf::Task profiler_before_ai_actions_task = m_coordinator.emplace([](){
            EASY_NONSCOPED_BLOCK("AI/actions", profiler::colors::Indigo600);
        }).name("profiler/before-ai-actions");
        tf::Task profiler_after_ai_actions_task = m_coordinator.emplace([](){
            EASY_END_BLOCK;
        }).name("profiler/after-ai-actions");
        profiler_before_ai_actions_task.precede(ai_actions_task);
        profiler_after_ai_actions_task.succeed(ai_actions_task);
        script_ai_task.succeed(profiler_after_ai_actions_task);
        maybe_ai_actions_task = profiler_before_ai_actions_task;
#else
        script_ai_task.succeed(ai_actions_task);
        maybe_ai_actions_task = ai_actions_task;
#endif
    }

    if (tf::Taskflow* ai_execute_flow = helpers::find_or(taskflows, Stage::AIExecute, nullptr)) {
        ai_execute_flow->name("Execute AI");
        tf::Task ai_execute_tasks = m_coordinator.composed_of(*ai_execute_flow).name("ai/execute");
#ifdef BUILD_WITH_EASY_PROFILER
        tf::Task profiler_before_ai_execute_task = m_coordinator.emplace([](){
            EASY_NONSCOPED_BLOCK("AI/execute", profiler::colors::Indigo600);
        }).name("profiler/before-ai-execute");
        tf::Task profiler_after_ai_execute_task = m_coordinator.emplace([](){
            EASY_END_BLOCK;
        }).name("profiler/after-ai-execute");
        before_ai_execute_task_profiler.precede(ai_execute_tasks);
        after_ai_execute_task_profiler.succeed(ai_execute_tasks);
        if (maybe_ai_actions_task.has_value()) {
            after_ai_execute_task_profiler.precede(maybe_ai_actions_task.value());
        } else {
            after_ai_execute_task_profiler.precede(ai_actions_task);
        }
#else
        if (maybe_ai_actions_task.has_value()) {
            ai_execute_tasks.precede(maybe_ai_actions_task.value());
        } else {
            ai_execute_tasks.precede(ai_actions_task);
        }
#endif
    }

#ifdef DEBUG_BUILD
    const bool dev_mode = entt::monostate<"game/dev-mode"_hs>();
    if (dev_mode) {
        std::ofstream file("task_graph.dot", std::ios_base::out);
        m_coordinator.dump(file);
    }
#endif
#endif
}
