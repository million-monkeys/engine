#pragma once

#include <monkeys.hpp>

namespace core {
    class Engine;
}

namespace scripting {

    namespace detail {
        using VariantVector = std::vector<std::variant<std::string,const char*,int,long,float,double,bool>>;
        void call (const std::string& function, const VariantVector& args);
    }

    bool init (core::Engine* engine);
    void registerComponent (entt::hashed_string::hash_type name, entt::id_type id);
    bool evaluate (const std::string& name, const std::string& source);
    bool load (const std::string& filename);
    template <typename... Args>
    void call (const std::string& function, Args... args) {
        const detail::VariantVector arguments{args...};
        detail::call(function, arguments);
    }
    

    void processGameEvents ();
    void processSceneEvents (million::resources::Handle handle);
    void processMessages ();

    void term ();

}
