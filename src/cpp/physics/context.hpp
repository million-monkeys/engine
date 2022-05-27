#pragma once

#include <monkeys.hpp>
#include <PxPhysicsAPI.h>

namespace physics {
    struct Context {
        modules::Context* m_modules_ctx;
        world::Context* m_world_ctx;

        float m_timestep_cccumulator;
        float m_step_size;
        unsigned m_frames_late;

        physx::PxDefaultAllocator m_default_allocator_callback;
        physx::PxDefaultErrorCallback  m_default_error_callback;
        physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxMaterial* m_material = nullptr;
        physx::PxPvd* m_pvd = nullptr;
    };
    
    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Teal900,
            profiler::colors::Teal700,
            profiler::colors::Teal500,
            profiler::colors::Teal300,
            profiler::colors::Teal100,
        };
        return colors[idx];
    }
}
