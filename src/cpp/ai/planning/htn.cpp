#include "ai/planner.hpp"

std::vector<Operator> htn::plan (const state::World& current, const Task* root_task, const Actions& vocabulary)
{
    immer::vector<const Action*> current_plan;
    state::World working_state{current};
    immer::vector<const Task*> tasks_to_process{root_task};
    std::vector<History> history;
    std::size_t methods_to_skip = 0;
    while (!tasks_to_process.empty()) {
        const Task* current_task = tasks_to_process.back();

        bool valid_task_found = false;
        if (const auto* task = std::get_if<CompoundTask>(current_task)) {                
            assert(methods_to_skip <= tasks_to_process.size());
            std::size_t index = 0;
            const CompoundTask::Method* current_best_method = nullptr;
            float current_best_score = 0.0f;
            std::size_t current_best_index = 0;
            for (auto it = task->methods.begin() + methods_to_skip; it != task->methods.end(); ++it) {
                const auto& method = *it;
                if (method.valid(working_state)) {
                    // TODO: find utility scores of valid methods
                    std::size_t utility_score = 0.0f;
                    if (current_best_method == nullptr || utility_score > current_best_score) {
                        current_best_method = &method;
                        current_best_index = index;
                        break;
                    }
                }
                ++index;
            }
            methods_to_skip = 0;
            if (current_best_method != nullptr) {
                // Add tasks to history stack
                history.push_back(History{current_plan, tasks_to_process, working_state, current_best_index});
                // Add subtasks to be processed
                for (const auto sub_task : current_best_method->subtasks) {
                    tasks_to_process = tasks_to_process.push_back(&sub_task->task);
                }
                valid_task_found = true;
            }
        } else if (const auto* primitive_task = std::get_if<PrimitiveTask>(current_task)) {
            const auto it = vocabulary.find(*primitive_task);
            if (it != vocabulary.end()) {
                const Action* action = it->second;
                if (action->valid(working_state)) {
                    // Apply operators effects
                    working_state = action->apply_effects(working_state);
                    // Add operator to plan
                    current_plan = current_plan.push_back(action);
                    valid_task_found = true;
                }
            }
        }
        // If no tasks preconditions are satisfied, restore from history, if there is any
        if (! valid_task_found) {
            if (!history.empty()) {
                const auto& restored = history.back();
                current_plan = restored.plan;
                tasks_to_process = restored.tasks_to_process;
                working_state = restored.state;
                methods_to_skip = restored.method_index;
                history.pop_back();
            } else {
                // There was no history, planning has failed
                return {};
            }
        } else {
            // A valid task was found and history did not need to be restored, pop the processed task from the stack
            tasks_to_process = tasks_to_process.take(tasks_to_process.size() - 1);
        }
    }
    
    // Copy current winning plan to vector of operators and return
    std::vector<Operator> final_plan;
    for (const auto* action : current_plan) {
        final_plan.emplace_back(Operator{action});
    }
    return final_plan;
}

SCENARIO("test") {
    GIVEN("A world state") {
        // state::World world{
        //     state::Facts{}
        //         .set("health"_hs, state::value_types::Real{100.0})
        //         .set("weapon", state::value_types::Identifier{"none"_hs})
        //         .set("status", state::value_types::Integer{0b000})
        // };
        // std::array<
        WHEN("There is a test") {
            THEN("it should test") {
                CHECK(0 == 0);
                CHECK(1 == 1);
            }
        }
    }
}
