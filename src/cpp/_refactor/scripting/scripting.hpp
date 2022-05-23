#pragma once

#include <monkeys.hpp>

namespace core {
    class Engine;
}

namespace scripting {
    namespace detail {
        using VariantVector = std::vector<std::variant<std::string,const char*,int,long,float,double,bool,void*>>;
        void call (Context* context, const std::string& function, const VariantVector& args);
    }

    Context* init (messages::Context* msgs_ctx, events::Context* events_ctx, resources::Context* resources_ctx);
    void setWorld (Context* context, world::Context* world_ctx);

    void registerComponent (Context* context, entt::hashed_string::hash_type name, entt::id_type id);
    bool evaluate (Context* context, const std::string& name, const std::string& source);
    bool load (Context* context, const std::string& filename);
    template <typename... Args>
    void call (Context* context, const std::string& function, Args... args) {
        const detail::VariantVector arguments{args...};
        detail::call(context, function, arguments);
    }
    

    void processGameEvents (Context* context);
    void processSceneEvents (Context* context, million::resources::Handle handle);
    void processMessages (Context* context);

    void term (Context* context);

}
