
#include "scheduler.hpp"
#include "engine.hpp"
#include "scripting/scripting.hpp"

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

void scheduler::Scheduler::createTaskGraph (core::Engine& engine)
{
    // Setup Systems by creating a Taskflow graph for each stage
    auto& registry = engine.registry(million::Registry::Runtime);
    phmap::flat_hash_map<million::SystemStage, tf::Taskflow*> taskflows;
    for (auto type : magic_enum::enum_values<million::SystemStage>()) {
        auto it = m_organizers.find(type);
        if (it != m_organizers.end()) {
            tf::Taskflow* taskflow = new tf::Taskflow();
            taskflows[type] = taskflow;
            std::vector<std::pair<entt::organizer::vertex, tf::Task>> tasks;
            auto graph = it->second.graph();
            // First pass, prepare registry and create taskflow task
            for(auto&& node : graph) {
                SPDLOG_DEBUG("Setting up system: {}", node.name());
                node.prepare(registry);
                auto callback = node.callback();
                auto userdata = node.data();
                auto name = node.name();
                tasks.push_back({
                    node,
                    taskflow->emplace([&registry, userdata, callback, name](){
                        (void)name; SPDLOG_TRACE("Running System: {}", name);
                        callback(userdata, registry);
                    }).name(node.name())
                });
                SPDLOG_DEBUG("System setup: {}", node.name());
            }
            SPDLOG_DEBUG("Setting up task relationships");
            // Second pass, set parent-child relationship of tasks
            for (auto&& [node, task] : tasks) {
                for (auto index : node.children()) {
                    task.precede(tasks[index].second);
                }
            }
            it->second.clear();
            SPDLOG_DEBUG("Tasks setup");
        }
    }
    SPDLOG_DEBUG("Clearing organizers");
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
     * // Copy current frames events for processing next frame
     * [*] = modules of subtasks
     **/

    tf::Task scripts_behavior = m_coordinator.emplace([&engine](){
        EASY_BLOCK("Scripts/behavior", profiler::colors::Purple100);
        scripting::processEvents(engine);
        engine.pumpScriptEvents();
    }).name("scripts/behavior");
    
    tf::Task scripts_ai = m_coordinator.emplace([](){
        EASY_BLOCK("Scripts/AI", profiler::colors::Purple100);
        // execute lua scripts here
    }).name("scripts/ai");

    tf::Task physics_prepare = m_coordinator.emplace([&registry, this](){
        EASY_BLOCK("Physics/prepare", profiler::colors::Purple100);
        physics::prepare(m_physics_context, registry);
    }).name("phycis/prepare");

    tf::Task physics_simulate = m_coordinator.emplace([this](){
        EASY_BLOCK("Physics/simulate", profiler::colors::Purple200);
        physics::simulate(m_physics_context);
    }).name("physics/simulate");

    tf::Task pump_events = m_coordinator.emplace([&engine](){
        // Copy current frames events for processing next frame
        engine.pumpEvents();
    }).name("events/pump");

    tf::Task before_update = m_coordinator.emplace([&engine](){
        // call before_update hooks here
        engine.callModuleHook<core::CM::BEFORE_UPDATE>();
    }).name("hooks/before-update");

    pump_events.precede(before_update);
    
    // Add engine-internal tasks to graph and coordinate flow into one graph
    if (tf::Taskflow* game_logic_flow = helpers::find_or(taskflows, million::SystemStage::GameLogic, nullptr)) {
        SPDLOG_DEBUG("Adding gamelogic to taskflow");
        game_logic_flow->name("Game Logic");
        tf::Task game_logic = m_coordinator.composed_of(*game_logic_flow).name("systems/game-logic");

        scripts_behavior.precede(game_logic);
        game_logic.precede(pump_events);

// #ifdef BUILD_WITH_EASY_PROFILER
//         tasks["profiler/before-game-logic"] = m_coordinator.emplace([](){
//             EASY_NONSCOPED_BLOCK("Systems/game-logic", profiler::colors::Indigo400);
//         });
//         tasks["profiler/after-game-logic"] = m_coordinator.emplace([](){EASY_END_BLOCK;});

//         connect("BEFORE", "profiler/before-game-logic");
//         connect("profiler/before-game-logic", "systems/game-logic");
//         connect("systems/game-logic", "profiler/after-game-logic");
//         connect("profiler/after-game-logic", "AFTER");
// #else
//         connect("BEFORE", "systems/game-logic");
//         connect("systems/game-logic", "AFTER");
// #endif
    } else {
//         connect("BEFORE", "AFTER");
            SPDLOG_DEBUG("No gamelogic to add");
            scripts_behavior.precede(pump_events);
    }

//     if (tf::Taskflow* updater_flow = helpers::find_or(taskflows, million::SystemStage::Update, nullptr)) {
//         updater_flow->name("State Update");
//         tasks["systems/update"] = m_coordinator.composed_of(*updater_flow);
// #ifdef BUILD_WITH_EASY_PROFILER
//         tasks["profiler/before-systems-update"] = m_coordinator.emplace([](){
//             EASY_NONSCOPED_BLOCK("Systems/update", profiler::colors::Indigo600);
//         });
//         tasks["profiler/before-systems-update"] = m_coordinator.emplace([](){EASY_END_BLOCK;});

//         connect("BEFORE", "profiler/before-systems-update");
//         connect("profiler/before-systems-update", "systems/update");
//         connect("systems/update", "profiler/after-systems-update");
//         connect("profiler/after-systems-update", "AFTER");
// #else
//         connect("BEFORE", "systems/update");
//         connect("systems/update", "AFTER");
// #endif
//     } else {
//         connect("BEFORE", "AFTER");
//     }

//     std::optional<tf::Task> maybe_ai_actions_task;
//     if (tf::Taskflow* ai_actions_flow = helpers::find_or(taskflows, million::SystemStage::Actions, nullptr)) {
//         ai_actions_flow->name("Apply Actions");
//         tasks["actions"] = m_coordinator.composed_of(*ai_actions_flow).name("actions");
// #ifdef BUILD_WITH_EASY_PROFILER
//         tasks["profiler/before-actions"] = m_coordinator.emplace([](){
//             EASY_NONSCOPED_BLOCK("AI/actions", profiler::colors::Indigo600);
//         });
//         tasks["profiler/after-actions"] = m_coordinator.emplace([](){
//             EASY_END_BLOCK;
//         });

//         connect("BEFORE", "profiler/before-actions");
//         connect("profiler/before-actions", "actions");
//         connect("actions", "profiler/after-actions");
//         connect("profiler/after-actions", "AFTER");
// #else
//         connect("BEFORE", "actions");
//         connect("actions", "AFTER");
// #endif
//     } else {
//         connect("BEFORE", "AFTER");
//     }

//     if (tf::Taskflow* ai_execute_flow = helpers::find_or(taskflows, million::SystemStage::AIExecute, nullptr)) {
//         ai_execute_flow->name("Execute AI");
//         tasks["ai/execute"] = m_coordinator.composed_of(*ai_execute_flow);
// #ifdef BUILD_WITH_EASY_PROFILER
//         tasks["profiler/before-ai-execute"] = m_coordinator.emplace([](){
//             EASY_NONSCOPED_BLOCK("AI/execute", profiler::colors::Indigo600);
//         });
//         tasks["profiler/after-ai-execute"] = m_coordinator.emplace([](){
//             EASY_END_BLOCK;
//         });

//         connect("BEFORE", "profiler/before-ai-execute");
//         connect("profiler/before-ai-execute", "ai/execute");
//         connect("ai/execute", "profiler/after-ai-execute");
//         connect("profiler/after-ai-execute", "AFTER");
// #else
//         connect("BEFORE", "ai/execute");
//         connect("ai/execute", "AFTER");
// #endif
//     } else {
//         connect("BEFORE", "AFTER");
//     }



// #ifdef DEBUG_BUILD
//     const bool dev_mode = entt::monostate<"game/dev-mode"_hs>();
//     if (dev_mode) {
        std::ofstream file("task_graph.dot", std::ios_base::out);
        m_coordinator.dump(file);
//     }
// #endif
}

void scheduler::Scheduler::execute ()
{
    EASY_BLOCK("Executing tasks", profiler::colors::Indigo200);
    m_executor.run(m_coordinator);
    m_executor.wait_for_all();
}
