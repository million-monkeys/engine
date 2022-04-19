#pragma once

#include <entt/fwd.hpp>

#include "types.hpp"
#include <entt/core/any.hpp>

namespace monkeys::api {
    class Engine;

    namespace definitions {
        struct Attribute {
            std::string name;
            monkeys::types::Type type;
            std::size_t offset;
            struct Option {
                std::string label;
                entt::any value; // Warning: must be careful to make sure this value is only ever accessed as the type specified by Attribute::type
            };
            std::vector<Option> options;
        };
        enum class ManageOperation {
            Add,
            Remove,
        };
        using LoaderFn = void(*)(Engine* engine, entt::registry& registry, const void* table, entt::entity entity);
        using CheckerFn = bool(*)(entt::registry& registry, entt::entity entity);
        using GetterFn = char*(*)(entt::registry& registry, entt::entity entity);
        using ManageFn = void(*)(entt::registry& registry, entt::entity entity, ManageOperation);
        struct Component {
            entt::hashed_string id;
            entt::id_type type_id;
            std::string category;
            std::string name;
            std::size_t size_in_bytes;
            LoaderFn loader;
            CheckerFn attached_to_entity;
            GetterFn getter;
            ManageFn manage;
            std::vector<Attribute> attributes;
        };
    }
}
