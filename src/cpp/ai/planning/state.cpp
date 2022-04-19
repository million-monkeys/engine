#include "ai/planner.hpp"

bool state::World::satisfies (const Conditions& conditions) const {
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
state::World state::World::goalState (const Conditions& conditions) const {
    World goal;
    for (const auto& condition : conditions) {
        auto comp = condition.comparator;
        auto result = std::visit(
            [comp](auto&& current, auto&& expected) -> value_types::Value {
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
                            return value_types::Real{value_types::Real::Type(expected.value) - current.value};
                        case Op::LessThan:
                        case Op::LessThanOrEqualTo:
                            return value_types::Real{value_types::Real::Type(current.value) - expected.value};
                        case Op::EqualTo:
                        case Op::NotEqualTo:
                            return value_types::Real{value_types::Real::Type(expected.value)};
                        default:
                            return value_types::Real{value_types::Real::Type(expected.value)};
                    };
                // } else if constexpr (std::is_same_v<Expected, value_types::Boolean>) {
                //     return value_types::Boolean{expected.value};
                // } else if constexpr (std::is_same_v<Expected, value_types::Enumeration>) {
                //     return value_types::Enumeration{expected.value};
                // } else if constexpr (std::is_same_v<Expected, value_types:Identifier>) {
                //     return value_types::Identifier{expected.value};
                } else {
                    return expected;
                }
            },
            facts[condition.fact],
            condition.value);
        goal.facts = goal.facts.set(condition.fact, result);
    }
    return goal;
}

// calculate the distance from one world state to another world state
float state::World::distanceTo (const World& goal, const Conditions& conditions) const {
    float total = 0.0f;
    for (const auto& condition : conditions) {
        auto comp = condition.comparator;
        auto key = condition.fact;
        const auto* value = goal.facts.find(key);
        if (value != nullptr) {
            float cost = std::visit(
                [comp](auto&& current, auto&& expected) -> float {
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
                *value);
            // Clamp to 0.0 .. 1.0 and add to total
            total += (cost > 1.0f) ? 1.0f : cost;
        }
    }
    return total;
}

const state::World state::World::apply (const Effects& effects) const {
    World world{facts};
    for (const auto& effect : effects) {
        auto comp = effect.op;
        auto result = std::visit(
            [comp](auto&& current, auto&& outcome) -> value_types::Value {
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
                            return value_types::Integer{value_types::Integer::Type(outcome.value)};
                        case Op::Add:
                            return value_types::Integer{current.value + value_types::Integer::Type(outcome.value)};
                        case Op::Subtract:
                            return value_types::Integer{current.value - value_types::Integer::Type(outcome.value)};
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