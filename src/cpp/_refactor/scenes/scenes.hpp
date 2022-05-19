#pragma once

#include <game.hpp>

namespace events { struct Context; }

namespace scenes {
    struct Context;

    Context* init (events::Context* events_ctx, resources::Context* resources_ctx, scripting::Context* scripting_ctx);
    void term (Context* context);

    void setContextData (Context* context, million::api::EngineRuntime* runtime);

    entt::entity loadEntity (Context* context, entt::hashed_string prototype_id);
    void mergeEntity (Context* context, entt::entity entity, entt::hashed_string prototype_id, bool overwrite_components);
    entt::entity findEntity (Context* context, entt::hashed_string name);
    const std::string& findEntityName (Context* context, const components::core::Named& named);
    bool isInGroup (Context* context, entt::entity entity, entt::hashed_string::hash_type group_name);

    void update (Context* context);
    void swapScenes (Context* context);
    void processEvents (Context* context);

    void loadSceneList (Context* context, const std::string& path);
    void loadScene (Context* context, entt::hashed_string::hash_type scene, bool auto_swap);

    entt::registry& registry (Context* context);
}
