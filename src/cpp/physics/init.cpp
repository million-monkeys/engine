#include "physics.hpp"
#include "context.hpp"

physics::Context* physics::init (world::Context* world_ctx)
{
    EASY_BLOCK("physics::init", physics::COLOR(1));
    SPDLOG_DEBUG("[physics] Init");
    auto context = new physics::Context{};
    context->m_world_ctx = world_ctx;

    context->m_timestep_cccumulator = 0.0f;
    context->m_step_size = 1.0f / 60.0f;

    context->m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, context->m_default_allocator_callback, context->m_default_error_callback);
    if (!context->m_foundation) {
        return nullptr;
    }
// #ifdef DEBUG_BUILD
//     context->m_pvd = PxCreatePvd(*context->m_foundation);
//     physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
//     context->m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
// #endif
    physx::PxTolerancesScale tolerance_scale;
    // tolerance_scale.length = 100;        // typical length of an object
    // tolerance_scale.speed = 981;         // typical speed of an object, gravity*1s is a reasonable choice
    context->m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *context->m_foundation, tolerance_scale, true, context->m_pvd);

    // static_friction, dynamic_friction, restitution 
    return context;
}

void physics::term (physics::Context* context)
{
    EASY_BLOCK("physics::term", physics::COLOR(1));
    SPDLOG_DEBUG("[physics] Term");
    if (context->m_scene) {
        context->m_scene->release();
    }
    if (context->m_dispatcher) {
        context->m_dispatcher->release();
    }
    context->m_physics->release();
    context->m_foundation->release();
    delete context;
}

void physics::createScene (physics::Context* context)
{
    EASY_BLOCK("physics::createScene", physics::COLOR(1));
    SPDLOG_DEBUG("[physics] Create Scene");
    physx::PxSceneDesc scene_desc (context->m_physics->getTolerancesScale());
    scene_desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f); // TODO: Read from game config
    context->m_dispatcher = physx::PxDefaultCpuDispatcherCreate(2); // TODO: Read from engine config
    scene_desc.cpuDispatcher = context->m_dispatcher;
    scene_desc.filterShader	= physx::PxDefaultSimulationFilterShader;
    context->m_scene = context->m_physics->createScene(scene_desc);

// #ifdef DEBUG_BUILD
//     physx::PxPvdSceneClient* pvd_client = context->m_scene->getScenePvdClient();
//     if(pvd_client) {
//         pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
//         pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
//         pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
//     }
// #endif
}
