#pragma once

#include <monkeys.hpp>

namespace world {
    Context* init (events::Context* events_ctx, messages::Context* messages_ctx, resources::Context* resources_ctx, scripting::Context* scripting_ctx);
    void term (Context* context);

    void setContextData (Context* context, million::api::EngineRuntime* runtime);
    void installComponent (Context* context, const million::api::definitions::Component& component, million::api::definitions::PrepareFn prepareFn);
    void setEntityCategories (Context* context, const std::vector<entt::hashed_string::hash_type>& entity_categories);

    entt::entity loadEntity (Context* context, entt::hashed_string prototype_id);
    void mergeEntity (Context* context, entt::entity entity, entt::hashed_string prototype_id, bool overwrite_components);
    entt::entity findEntity (Context* context, entt::hashed_string name);
    const std::string& findEntityName (Context* context, const components::core::Named& named);
    bool isInGroup (Context* context, entt::entity entity, entt::hashed_string::hash_type group_name);
    std::uint16_t categoryBitflag (Context* context, entt::hashed_string::hash_type category_name);

    void update (Context* context);
    void swapScenes (Context* context);
    void processEvents (Context* context);
    void executeHandlers (Context* context);

    void loadSceneList (Context* context, const std::string& path);
    void loadScene (Context* context, entt::hashed_string::hash_type scene, bool auto_swap);

    entt::registry& registry (Context* context);
}
