#include "world.hpp"
#include "context.hpp"
#include "utils/parser.hpp"
#include "_refactor/resources/resources.hpp"

#include <physfs.hpp>

void world::loadSceneList (world::Context* context, const std::string& path)
{
    EASY_FUNCTION(profiler::colors::Pink100);
    // Clear previous scenes, if any
    context->m_scenes.clear();

    context->m_path = path;
    for (const auto& filename : physfs::enumerateFiles(path)) {
        auto file = std::filesystem::path(filename);
        if (file.extension() == ".toml") {
            auto scene = file.replace_extension("").string();
            auto scene_id = entt::hashed_string::value(scene.c_str());
            SPDLOG_DEBUG("[scenes] Adding scene: {} ({:x})", scene, scene_id);
            context->m_scenes[scene_id] = scene;
        }
    }
}

void world::loadScene (world::Context* context, entt::hashed_string::hash_type scene, bool auto_swap)
{
    EASY_FUNCTION(profiler::colors::RichYellow);

    auto it = context->m_scenes.find(scene);
    if (it != context->m_scenes.end()) {        
        const auto scene_name = it->second;
        const auto filename = (context->m_path / scene_name).replace_extension("toml").string();

        spdlog::info("[scenes] Loading scene: {}", scene_name);
        const auto config = parser::parse_toml(filename);
        PendingScene& pending = context->m_pending_scenes[scene];
        pending.auto_swap = auto_swap;

        // Load entities and prototypes
        if (config.contains("entity")) {
            auto handle = resources::load(context->m_resources_ctx, "scene-entities"_hs, filename, scene);
            pending.resources.insert(handle.handle);
        }

        // Preload resources
        if (config.contains("resources")) {
            for (const auto& [name, resource] : config.at("resources").as_table()) {
                if (resource.is_table() && resource.contains("type") && resource.contains("file")) {
                    auto& type = resource.at("type");
                    auto& file = resource.at("file");
                    if (! type.is_string()) {
                        spdlog::error("[scenes] In scene file '{}', resource '{}' has invalid type field", scene_name, name);
                        continue;
                    }
                    if (! file.is_string()) {
                        spdlog::error("[scenes] In scene file '{}', resource '{}' has invalid file field", scene_name, name);
                        continue;
                    }
                    // Queue resource for loading
                    auto handle = resources::load(context->m_resources_ctx, entt::hashed_string{type.as_string().str.c_str()}, file.as_string(), scene);
                    // Bind resource to name
                    resources::bindToName(context->m_resources_ctx, handle, entt::hashed_string::value(name.c_str()));
                    // Add resoruce to pending for scene
                    pending.resources.insert(handle.handle);
                }
            }
        }

        // Load scripts
        if (config.contains("script-file") && config.contains("event-map")) {
            auto handle = resources::load(context->m_resources_ctx, "scene-script"_hs, filename, scene);
            pending.resources.insert(handle.handle);
        }

    } else {
        spdlog::error("[scenes] Could not load scene because it does not exist: {}", scene);
    }
}