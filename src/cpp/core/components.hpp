#pragma once

namespace core {
    // Component used by the prototypes registry to identify the prototype entity
    struct EntityPrototypeID {
        entt::hashed_string::hash_type id;
    };

    // Component used to represent group membership. Component named storage is used for different groups.
    struct EntityGroup {};
}