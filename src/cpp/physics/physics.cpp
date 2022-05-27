#include "physics.hpp"
#include "context.hpp"

#include "modules/modules.hpp"

void physics::simulate (physics::Context* context, timing::Time current, timing::Delta delta)
{
    context->m_timestep_cccumulator += delta;
    if(context->m_timestep_cccumulator < context->m_step_size) {
        return;
    }
    // If more than 1/4 second has passed, just take the hit on jitter by dropping steps
    if(context->m_timestep_cccumulator > 0.25f) { // TODO: Make this an engine config
        context->m_timestep_cccumulator = context->m_step_size;
    }
    // Run one time step
    context->m_timestep_cccumulator -= context->m_step_size;
    modules::hooks::physics_step(context->m_modules_ctx, context->m_step_size);
    if (context->m_timestep_cccumulator >= context->m_step_size) {
        // Still time, count this frame
        if (++context->m_frames_late >= 5) { // TODO: Make this an engine config
            // After five consecutive late frames, just take the hit on jitter
            context->m_timestep_cccumulator = context->m_step_size;
            context->m_frames_late = 0;
        }
        do {
            context->m_timestep_cccumulator -= context->m_step_size;
            modules::hooks::physics_step(context->m_modules_ctx, context->m_step_size);
        } while (context->m_timestep_cccumulator >= context->m_step_size);
    } else {
        context->m_frames_late = 0;
    }
        
    // TODO: Move to physx-module
    // context->m_scene->simulate(context->m_step_size);
    // context->m_scene->fetchResults(true);
}
