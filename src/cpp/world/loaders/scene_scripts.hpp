#pragma once

#include <monkeys.hpp>
#include "resources/resources.hpp"

namespace loaders {
    class SceneScripts : public million::api::resources::Loader {
    public:
        SceneScripts(world::Context* context) : m_context(context) {}
        virtual ~SceneScripts() {}

        bool cached (const std::string& filename, std::uint32_t*) final; // scene-script's are cached
        bool load (million::resources::Handle handle, const std::string& filename) final;
        void unload (million::resources::Handle handle) final;
        
        entt::hashed_string name () const final { return Name; }
        static constexpr entt::hashed_string Name = "scene-script"_hs;

    private:
        world::Context* m_context;
        helpers::hashed_string_map<std::uint32_t, helpers::thread_safe_flat_map> m_cached_ids;
    };
}
