#pragma once

#include <monkeys.hpp>

namespace modules {
    Context* init ();
    void term (Context* context);

    void addHook (Context* context, million::api::Module::CallbackMasks hook, million::api::Module* mod);

    namespace hooks {
        void before_frame (Context* context, timing::Time, timing::Delta, uint64_t);
        void before_update (Context* context);
        void after_frame (Context* context);
        void prepare_render (Context* context);
        void before_render (Context* context);
        void after_render (Context* context);
        void load_scene (Context* context, entt::hashed_string::hash_type scene_id, const std::string& name);
        void unload_scene (Context* context, entt::hashed_string::hash_type scene_id, const std::string& name);
    }
}
