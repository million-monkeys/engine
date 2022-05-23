#pragma once

#include <monkeys.hpp>

struct RegistryPair {
public:
    enum class Registries {
        Background,
        Foreground,
    };

    struct NamedEntityInfo {
        entt::entity entity;
        std::string name;
    };

    RegistryPair();
    ~RegistryPair();
    entt::registry runtime;
    entt::registry prototypes;
    helpers::hashed_string_flat_map<NamedEntityInfo> entity_names;
    helpers::hashed_string_flat_map<entt::entity> prototype_names;

    void clear ();
private:
    // Callbacks to manage Named entities
    void onAddNamedEntity (entt::registry&, entt::entity);
    void onRemoveNamedEntity (entt::registry&, entt::entity);

    // Callbacks to manage prototype entities
    void onAddPrototypeEntity (entt::registry&, entt::entity);
    void onRemovePrototypeEntity (entt::registry&, entt::entity);
};
