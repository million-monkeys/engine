#include "world.hpp"
#include "context.hpp"

#include "scripting/scripting.hpp"
#include "modules/modules.hpp"

void world::prepareRegistries (world::Context* context, million::api::definitions::PrepareFn prepareFn)
{
    // Create storage for the component
    prepareFn(context->m_registries.foreground().runtime);
    prepareFn(context->m_registries.foreground().prototypes);
    prepareFn(context->m_registries.background().runtime);
    prepareFn(context->m_registries.background().prototypes);
}

void world::installComponent (world::Context* context, const million::api::definitions::Component& component, million::api::definitions::PrepareFn prepareFn)
{
    world::prepareRegistries(context, prepareFn);
    // Make component accessible from scripting
    scripting::registerComponent(context->m_scripting_ctx, component.id.value(), component.type_id);
    // Add component loader
    context->m_component_loaders[component.id] = component.loader;
}

void loadComponent (world::Context* context, entt::registry& registry, entt::hashed_string component, entt::entity entity, const void* table)
{
    EASY_FUNCTION(world::COLOR(4));
    auto it = context->m_component_loaders.find(component);
    if (it != context->m_component_loaders.end()) {
        const auto& loader = it->second;
        loader(modules::api_engine_setup(context->m_modules_ctx), registry, table, entity);
    } else {
        spdlog::warn("Tried to load non-existent component: {}", component.data());
    }
}