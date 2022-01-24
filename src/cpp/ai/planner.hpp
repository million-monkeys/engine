#pragma once

#include <game.hpp>
#include <immer/vector.hpp>
#include <immer/map.hpp>

namespace ai::planner {

    struct Action {
        struct Parameter {
            Parameter (entt::hashed_string k, std::variant<float, std::int64_t, entt::hashed_string::hash_type, entt::entity> v) : key(k.value()), value(v) {}
            entt::hashed_string::hash_type key;
            std::variant<float, std::int64_t, entt::hashed_string::hash_type, entt::entity> value;
        };

        entt::hashed_string::hash_type id;
        helpers::InlineArray<Parameter> parameters;

        template <class Allocator>
        static Action* create (Allocator& allocator, entt::hashed_string action_id, std::vector<Parameter>&& parameters) {
            std::size_t len = sizeof(Action) + (parameters.size() * sizeof(Parameter));
            Action* action = new (allocator.allocate(len)) Action{};
            action->id = action_id.value();
            action->parameters.fill(parameters);
            return action;
        }
        template <class Allocator = helpers::DefaultAllocator>
        static Action* create (entt::hashed_string action_id, std::vector<Parameter>&& parameters) {
            return create<Allocator::Object>(Allocator::object(), action_id, parameters);
        }

        template <class Allocator>
        static void destroy (Allocator& allocator, Action* action) {
            allocator.deallocate(reinterpret_cast<std::byte*>(action));
        }
        template <class Allocator = helpers::DefaultAllocator>
        static void destroy (Action* action) {
            destroy<Allocator::Object>(Allocator::object(), action);
        }
    };

    using Actions = helpers::InlineArray<Action>;

}

namespace state {
    namespace value_types {
        struct Integer {
            using Type = std::int32_t;
            Type value;
        };
        struct Real {
            using Type = float;
            Type value;
        };
        struct Boolean { bool value; };
        struct Enumeration { std::uint32_t value; };
        struct Identifier { entt::hashed_string::hash_type value; };

        enum class Name : entt::hashed_string::hash_type {
            Integer = "integer"_hs,
            Real = "real"_hs,
            Boolean = "boolean"_hs,
            Enumeration = "enumeration"_hs,
            Identifier = "identifier"_hs,
        };
        using Value = std::variant<value_types::Integer, value_types::Real, value_types::Boolean, value_types::Enumeration, Identifier>;

        Name name (const Value& value) {
            return std::visit(
                helpers::visitor{
                    [](const Integer&){ return Name::Integer; },
                    [](const Real&){ return Name::Real; },
                    [](const Boolean&){ return Name::Boolean; },
                    [](const Enumeration&){ return Name::Enumeration; },
                    [](const Identifier&) { return Name::Identifier; },
                },
                value);
        }
    }

    namespace conditions {
        enum class Operator {
            // For all types
            EqualTo,
            NotEqualTo,
            // For value_types::Name::{Integer, Real}
            GreaterThan,
            GreaterThanOrEqualTo,
            LessThan,
            LessThanOrEqualTo,
            // For value_types::Name::Integer (treat integer as bitfield)
            BitsAllSet,
            BitsSomeSet,
            BitsNoneSet,
        };
        struct Condition {
            entt::hashed_string::hash_type fact;
            Operator comparator;
            value_types::Value value;
        };
    }
    using Conditions = std::vector<conditions::Condition>;

    namespace effects {
        enum class Operator  {
            // For all types
            Set,
            // For value_types::Name::{Integer, Real}
            Add,
            Subtract,
            // For value_types::Name::Integer (treat integer as bitfield)
            SetBits,
            ClearBits,
            ToggleBits,
        };
        struct Effect {
            entt::hashed_string::hash_type fact;
            Operator op;
            value_types::Value value;
        };
    }
    using Effects = std::vector<effects::Effect>;

    struct World {
        //helpers::hashed_string_map<value_types::Value> facts;
        immer::map<entt::hashed_string::hash_type, value_types::Value> facts;

        bool satisfies (const Conditions& conditions) const {
            for (const auto& condition : conditions) {
                auto comp = condition.comparator;
                bool met = std::visit(
                    [comp](auto&& current, auto&& expected) {
                        using Op = conditions::Operator;
                        using Current = std::decay_t<decltype(current)>;
                        using Expected = std::decay_t<decltype(expected)>;                    
                        if constexpr ((std::is_same_v<Current, value_types::Integer> || std::is_same_v<Current, value_types::Real>) &&
                                    (std::is_same_v<Expected, value_types::Integer> || std::is_same_v<Expected, value_types::Real>) ) {
                            switch (comp) {
                                case Op::GreaterThan:
                                    return current.value > expected.value;
                                case Op::GreaterThanOrEqualTo:
                                    return current.value >= expected.value;
                                case Op::LessThan:
                                    return current.value < expected.value;
                                case Op::LessThanOrEqualTo:
                                    return current.value <= expected.value;
                                case Op::EqualTo:
                                    return current.value == expected.value;
                                case Op::NotEqualTo:
                                    return current.value != expected.value;
                                case Op::BitsAllSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        return (current.value & expected.value) == expected.value;
                                    } else {
                                        return false;
                                    }
                                case Op::BitsSomeSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        return bool(current.value & expected.value);
                                    } else {
                                        return false;
                                    }
                                case Op::BitsNoneSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        return !bool(current.value & expected.value);
                                    } else {
                                        return false;
                                    }
                                default:
                                    return false;
                            };
                        } else if constexpr (std::is_same_v<Current, Expected>) {
                            switch (comp) {
                                case Op::EqualTo:
                                    return current.value == expected.value;
                                case Op::NotEqualTo:
                                    return current.value != expected.value;
                                default:
                                    return false;
                            };
                        } else {
                            return false;
                        }
                    },
                    facts[condition.fact],
                    condition.value);
                if (! met) {
                    return false;
                }
            }
            return true;
        }

        // use to generate a world state from a set of conditions, for the purpose of determining distance
        World goalState (const Conditions& conditions) const {
            World goal;
            for (const auto& condition : conditions) {
                auto comp = condition.comparator;
                auto result = std::visit(
                    [comp](auto&& current, auto&& expected) {
                        using Op = conditions::Operator;
                        using Current = std::decay_t<decltype(current)>;
                        using Expected = std::decay_t<decltype(expected)>;
                        if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) { 
                            switch (comp) {
                                case Op::GreaterThan:
                                case Op::GreaterThanOrEqualTo:
                                    return value_types::Integer{expected.value - current.value};
                                case Op::LessThan:
                                case Op::LessThanOrEqualTo:
                                    return value_types::Integer{current.value - expected.value};
                                case Op::EqualTo:
                                case Op::NotEqualTo:
                                    return value_types::Integer{expected.value};
                                case Op::BitsAllSet:
                                case Op::BitsSomeSet:
                                case Op::BitsNoneSet:
                                    return value_types::Integer{expected.value};
                            };
                        } else if constexpr ((std::is_same_v<Current, value_types::Integer> || std::is_same_v<Current, value_types::Real>) &&
                                    (std::is_same_v<Expected, value_types::Integer> || std::is_same_v<Expected, value_types::Real>) ) {
                            switch (comp) {
                                case Op::GreaterThan:
                                case Op::GreaterThanOrEqualTo:
                                    return value_types::Real{value_types::Real::Type{expected.value} - current.value};
                                case Op::LessThan:
                                case Op::LessThanOrEqualTo:
                                    return value_types::Real{value_types::Real::Type{current.value} - expected.value};
                                case Op::EqualTo:
                                case Op::NotEqualTo:
                                    return value_types::Real{value_types::Real::Type{expected.value}};
                                default:
                                    return value_types::Real{value_types::Real::Type{expected.value}};
                            };
                        } else {
                            return Expected{expected.value};
                        }
                    },
                    facts[condition.fact],
                    condition.value);
                goal.facts = goal.facts.set(condition.fact, result);
            }
            return goal;
        }

        // calculate the distance from one world state to another world state
        float distanceTo (const World& goal) const {
            float total = 0.0f;
            for (const auto& [key, value] : goal.facts) {
                float value = std::visit(
                    [](auto&& current, auto&& expected) {
                        using Op = conditions::Operator;
                        using Current = std::decay_t<decltype(current)>;
                        using Expected = std::decay_t<decltype(expected)>;                    
                        if constexpr ((std::is_same_v<Current, value_types::Integer> || std::is_same_v<Current, value_types::Real>) &&
                                    (std::is_same_v<Expected, value_types::Integer> || std::is_same_v<Expected, value_types::Real>) ) {
                            switch (comp) {
                                case Op::GreaterThan:
                                    return current.value / expected.value;
                                case Op::GreaterThanOrEqualTo:
                                    return current.value / expected.value;
                                case Op::LessThan:
                                    return expected.value / current.value;
                                case Op::LessThanOrEqualTo:
                                    return expected.value / expected.value;
                                case Op::EqualTo:
                                    return current.value == expected.value ? 0.0f : 1.0f;
                                case Op::NotEqualTo:
                                    return current.value != expected.value ? 0.0f : 1.0f;
                                case Op::BitsAllSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        // TODO: Should this count how many are set?
                                        return (current.value & expected.value) == expected.value ? 0.0f : 1.0f;
                                    } else {
                                        return 1.0f;
                                    }
                                case Op::BitsSomeSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        return (current.value & expected.value) ? 0.0f : 1.0f;
                                    } else {
                                        return 1.0f;
                                    }
                                case Op::BitsNoneSet:
                                    if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Expected, value_types::Integer>) {
                                        // TODO: Should this count how many are not set?
                                        return !bool(current.value & expected.value) ? 0.0f : 1.0f;
                                    } else {
                                        return 1.0f;
                                    }
                                default:
                                    return 1.0f;
                            };
                        } else if constexpr (std::is_same_v<Current, Expected>) {
                            switch (comp) {
                                case Op::EqualTo:
                                    return current.value == expected.value ? 0.0f : 1.0f;
                                case Op::NotEqualTo:
                                    return current.value != expected.value ? 0.0f : 1.0f;
                                default:
                                    return 1.0f;
                            };
                        } else {
                            return 1.0f;
                        }
                    },
                    facts[key],
                    value);
                // Clamp to 0.0 .. 1.0 and add to total
                total += (value > 1.0f) ? 1.0f : value;
            }
            return total;
        }

        const World apply (const Effects& effects) const {
            World world{facts};
            for (const auto& effect : effects) {
                auto comp = effect.op;
                auto result = std::visit(
                    [comp](auto&& current, auto&& outcome) {
                        using Op = effects::Operator;
                        using Current = std::decay_t<decltype(current)>;
                        using Outcome = std::decay_t<decltype(outcome)>;
                        if constexpr (std::is_same_v<Current, value_types::Integer> && std::is_same_v<Outcome, value_types::Integer>) { 
                            switch (comp) {
                                case Op::Set:
                                    return value_types::Integer{outcome.value};
                                case Op::Add:
                                    return value_types::Integer{current.value + outcome.value};
                                case Op::Subtract:
                                    return value_types::Integer{current.value - outcome.value};
                                case Op::SetBits:
                                    return value_types::Integer{current.value | outcome.value};
                                case Op::ClearBits:
                                    return value_types::Integer{current.value & ~outcome.value};
                                case Op::ToggleBits:
                                    return value_types::Integer{current.value ^ outcome.value};
                            };
                        } else if constexpr ((std::is_same_v<Current, value_types::Integer> && std::is_same_v<Outcome, value_types::Real>)) {
                            switch (comp) {
                                case Op::Set:
                                    return value_types::Integer{value_types::Integer::Type{outcome.value}};
                                case Op::Add:
                                    return value_types::Integer{current.value + value_types::Integer::Type{outcome.value}};
                                case Op::Subtract:
                                    return value_types::Integer{current.value - value_types::Integer::Type{outcome.value}};
                                default:
                                    return current;
                            };
                        } else if constexpr ((std::is_same_v<Current, value_types::Real> && std::is_same_v<Outcome, value_types::Real>)) {
                            switch (comp) {
                                case Op::Set:
                                    return value_types::Real{outcome.value};
                                case Op::Add:
                                    return value_types::Real{current.value + outcome.value};
                                case Op::Subtract:
                                    return value_types::Real{current.value - outcome.value};
                                default:
                                    return current;
                            };
                        } else if constexpr (std::is_same_v<Current, Outcome>) {
                            if (comp == Op::Set) {
                                return Current{outcome.value};
                            }
                        }
                        return current;
                    },
                    facts[effect.fact],
                    effect.value);
                world.facts = world.facts.set(effect.fact, result);
            }
            return world;
        }
    };
}

using Parameters = immer::map<entt::hashed_string::hash_type, state::value_types::Value>;

struct Action {
    // Does the world state meet this acitons conditions?
    bool valid (const state::World state) const {
        return state.satisfies(preconditions);
    }

    // Apply the effects of this action to world state
    state::World apply_effects (const state::World state) const {
        return state.apply(effects).apply(expected_effects);
    }

    // Operator
    const state::Conditions preconditions;
    const entt::hashed_string action;
    const Parameters parameters;
    const state::Effects effects;
    const state::Effects expected_effects;
};

struct Operator {
private:
    const state::Conditions& preconditions;
    const state::Effects& effects;

public:
    const entt::hashed_string action;
    const Parameters& parameters;

    // Does the world state meet this acitons conditions?
    bool valid (const state::World state) const {
        return state.satisfies(preconditions);
    }

    // Apply the effects of this action to world state
    state::World apply_effects (const state::World state) const {
        return state.apply(effects);
    }
};

using Actions = helpers::hashed_string_map<Action>;

namespace goap {

    std::vector<ai::planner::Action> plan (const state::World& start, const state::World& goal, const std::vector<ai::planner::Action*>& vocabulary)
    {
        // A* search...
        return {};
    }

}

namespace htn {
    using PrimitiveTask = entt::hashed_string;
    struct CompoundTask {
        struct Method {
            state::Conditions preconditions;
            using SubTask = std::variant<PrimitiveTask, CompoundTask>;
            std::vector<SubTask> subtasks;
        };
        std::vector<Method> methods;
    };
    using Task = std::variant<PrimitiveTask, CompoundTask>;

    struct History {
        const immer::vector<const Action> plan;
        const immer::vector<const Task*> tasks_to_process;
        const state::World state;
    };

    std::vector<Operator> plan (const state::World& current, const Task* root_task, Actions& vocabulary)
    {
        immer::vector<const Action> current_plan;
        state::World working_state{current};
        immer::vector<const Task*> tasks_to_process{root_task};
        std::vector<const History> history;
        while (!tasks_to_process.empty()) {
            const Task* current_task = tasks_to_process.back();
            tasks_to_process = tasks_to_process.take(tasks_to_process.size() - 1);

            bool valid_task_found = false;
            if (const auto* task = std::get_if<CompoundTask>(current_task)) {
                for (const auto& method : task->methods) {
                    if (working_state.satisfies(method.preconditions)) {
                        // Add tasks to history stack
                        history.push_back(History{current_plan, tasks_to_process, working_state});
                        // Add subtasks to be processed
                        for (const auto& task : method.subtasks) {
                            tasks_to_process = tasks_to_process.push_back(&task);
                        }
                        valid_task_found = true;
                        break;
                    }
                }
            } else if (const auto* primitive_task = std::get_if<PrimitiveTask>(current_task)) {
                const auto& task = vocabulary.find()
                if (task->valid(working_state)) {
                    // Apply operators effects
                    working_state = task->apply_effects(working_state);
                    // Add operator to plan
                    current_plan = current_plan.push_back(task->action_operator);
                    valid_task_found = true;
                }
            }
            // If no tasks preconditions are satisfied, restore from history, if there is any
            if (! valid_task_found) {
                if (!history.empty()) {
                    const auto& restored = history.back();
                    current_plan = restored.plan;
                    tasks_to_process = restored.tasks_to_process;
                    working_state = restored.state;
                    history.pop_back();
                } else {
                    // There was no history, planning has failed
                    return {};
                }
            }
        }
        
        // Copy current winning plan to vector of operators and return
        std::vector<Operator> final_plan;
        for (const auto& action : current_plan) {
            final_plan.push_back(Operator{action.preconditions, action.effects, action.action, action.parameters});
        }
        return final_plan;
    }
}
