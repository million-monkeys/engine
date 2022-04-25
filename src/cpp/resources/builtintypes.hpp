#pragma once

#include "resources.hpp"

namespace resources::types {

    class ScriptedEvents : public monkeys::api::resources::Loader {
    public:
        virtual ~ScriptedEvents() {}
        bool load (monkeys::resources::Handle handle, const std::string& filename) final;
        void unload (monkeys::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scripted-events"_hs;
    };

}
