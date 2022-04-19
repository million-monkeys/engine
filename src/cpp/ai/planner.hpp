#pragma once

#include <game.hpp>
#include <immer/vector.hpp>
#include <immer/map.hpp>

// namespace ai::planner {

//     struct Action {
//         struct Parameter {
//             Parameter (entt::hashed_string k, std::variant<float, std::int64_t, entt::hashed_string::hash_type, entt::entity> v) : key(k.value()), value(v) {}
//             entt::hashed_string::hash_type key;
//             std::variant<float, std::int64_t, entt::hashed_string::hash_type, entt::entity> value;
//         };

//         entt::hashed_string::hash_type id;
//         helpers::InlineArray<Parameter> parameters;

//         template <class Allocator>
//         static Action* create (Allocator& allocator, entt::hashed_string action_id, std::vector<Parameter>&& parameters) {
//             std::size_t len = sizeof(Action) + (parameters.size() * sizeof(Parameter));
//             Action* action = new (allocator.allocate(len)) Action{};
//             action->id = action_id.value();
//             action->parameters.fill(parameters);
//             return action;
//         }
//         template <class Allocator = helpers::DefaultAllocator>
//         static Action* create (entt::hashed_string action_id, std::vector<Parameter>&& parameters) {
//             return create<Allocator::Object>(Allocator::object(), action_id, parameters);
//         }

//         template <class Allocator>
//         static void destroy (Allocator& allocator, Action* action) {
//             allocator.deallocate(reinterpret_cast<std::byte*>(action));
//         }
//         template <class Allocator = helpers::DefaultAllocator>
//         static void destroy (Action* action) {
//             destroy<Allocator::Object>(Allocator::object(), action);
//         }
//     };

//     using Actions = helpers::InlineArray<Action>;

// }

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

        inline Name name (const Value& value) {
            return std::visit(
                helpers::visitor{
                    [](const Integer&) { return Name::Integer; },
                    [](const Real&) { return Name::Real; },
                    [](const Boolean&) { return Name::Boolean; },
                    [](const Enumeration&) { return Name::Enumeration; },
                    [](const Identifier&) { return Name::Identifier; },
                },
                value);
        }
    }

    using Facts = immer::map<entt::hashed_string::hash_type, value_types::Value>;

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
        Facts facts;

        bool satisfies (const Conditions& conditions) const;

        // use to generate a world state from a set of conditions, for the purpose of determining distance
        World goalState (const Conditions& conditions) const;

        // calculate the distance from one world state to another world state
        float distanceTo (const World& goal, const Conditions& conditions) const;

        // Generate new world state by applying effects to this world stat
        const World apply (const Effects& effects) const;
    };
}

using Parameters = state::Facts;

struct Action {
private:
    // Operator
    const state::Conditions preconditions;
    const entt::hashed_string action;
    const Parameters parameters;
    const state::Effects effects;
    const state::Effects expected_effects;

public:
    // Does the world state meet this acitons conditions?
    bool valid (const state::World state) const {
        return state.satisfies(preconditions);
    }

    // Apply the effects of this action to world state
    state::World apply_effects (const state::World state) const {
        return state.apply(effects).apply(expected_effects);
    }

    // const Parameters get_parameters () const { return parameters; }

    friend struct Operator;
};

struct Operator {
private:
    const Action* m_action;

public:
    Operator (const Action* action) : m_action(action) {}
    ~Operator () = default;

    const entt::hashed_string action () const {
        return m_action->action;
    }
    const Parameters& parameters () const {
        return m_action->parameters;
    }

    // Does the world state meet this acitons conditions?
    bool valid (const state::World state) const {
        return state.satisfies(m_action->preconditions);
    }

    // Apply the effects of this action to world state
    state::World apply_effects (const state::World state) const {
        return state.apply(m_action->effects);
    }
};

using Actions = helpers::hashed_string_map<const Action*>;

namespace goap {

    std::vector<Action> plan (const state::World& start, const state::World& goal, const Actions& vocabulary);

}

namespace htn {
    // namespace ng {
    //     struct Task {
    //         virtual void handle () {}
    //     };
    //     struct Method {
    //         const state::Conditions preconditions;
    //         const std::vector<Task*> subtasks;

    //         bool valid (const state::World& state) const {
    //             return state.satisfies(preconditions);
    //         }
    //     };

    //     struct CompoundTask : public Task {
    //         const std::vector<Method> methods;
    //         void handle ();
    //     };
    //     struct PrimitiveTask : public Task {
    //         void handle ();
    //     };

    //     struct HTN {

    //         std::vector<Task*> tasks;
    //         helpers::hashed_string_map<Task*> domains;
    //     };
    // }
    using PrimitiveTask = entt::hashed_string;
    struct CompoundTask {
        struct Method {
            const state::Conditions preconditions;
            std::vector<struct SubTask*> subtasks;

            bool valid (const state::World& state) const {
                return state.satisfies(preconditions);
            }
        };
        std::vector<Method> methods;
    };
    using Task = std::variant<PrimitiveTask, CompoundTask>;
    struct SubTask {
        const Task& task;
    };

    struct History {
        const immer::vector<const Action*> plan;
        const immer::vector<const Task*> tasks_to_process;
        const state::World state;
        const std::size_t method_index;
    };

    std::vector<Operator> plan (const state::World& current, const Task* root_task, const Actions& vocabulary);
}
