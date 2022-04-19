#pragma once

#include <entt/core/hashed_string.hpp>
#include <glm/glm.hpp>

namespace monkeys {

    namespace types {
        enum class Type {
            Vec2,
            Vec3,
            Vec4,
            UInt8,
            UInt16,
            UInt32,
            UInt64,
            Int8,
            Int16,
            Int32,
            Int64,
            Byte,
            Flags8,
            Flags16,
            Flags32,
            Flags64,
            Resource,
            TextureResource,
            MeshResource,
            Entity,
            Float,
            Double,
            Bool,
            Event,
            Ref,
            HashedString,
            RGB,
            RGBA,
            Signal,
        };
    }

    namespace resources {
        struct Handle {};
    }
}
