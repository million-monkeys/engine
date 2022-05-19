#pragma once

#include <game.hpp>

namespace resources {
    struct Context;

    Context* init (events::Context* events_ctx);
    void term (Context* context);
    void poll (Context* context);

    void install (Context* context, million::api::resources::Loader* loader, bool managed=false);
    template <typename T, typename... Args> void install (Context* context, Args... args) { install(new T(args...), true); }
    
    million::resources::Handle load (Context* context, entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name=0);
    million::resources::Handle find (Context* context, entt::hashed_string::hash_type name);
    void bindToName (Context* context, million::resources::Handle handle, entt::hashed_string::hash_type name);

}
