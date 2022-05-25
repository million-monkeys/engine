#include "physics.hpp"
#include "context.hpp"

void physics::prepare (physics::Context* context, entt::registry& registry)
{

}

void physics::simulate (physics::Context* context, float delta)
{
    context->m_timestep_cccumulator += delta;
    if(context->m_timestep_cccumulator < context->m_step_size) {
        return;
    }
    context->m_timestep_cccumulator -= context->m_step_size;
    context->m_scene->simulate(context->m_step_size);
    context->m_scene->fetchResults(true);
    return;
}
