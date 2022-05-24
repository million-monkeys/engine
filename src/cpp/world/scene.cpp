#include "world.hpp"
#include "context.hpp"
#include "utils/parser.hpp"

#include "resources/resources.hpp"
#include "modules/modules.hpp"

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
            SPDLOG_DEBUG("[world] Adding scene: {} ({:x})", scene, scene_id);
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

        spdlog::info("[world] Loading scene: {}", scene_name);
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
                        spdlog::error("[world] In scene file '{}', resource '{}' has invalid type field", scene_name, name);
                        continue;
                    }
                    if (! file.is_string()) {
                        spdlog::error("[world] In scene file '{}', resource '{}' has invalid file field", scene_name, name);
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
        spdlog::error("[world] Could not load scene because it does not exist: {}", scene);
    }
}

// Swap foreground and background scenes, and clear the (new) background scene
void world::swapScenes (world::Context* context)
{
    EASY_FUNCTION(profiler::colors::Amber800);
    SPDLOG_TRACE("[world] Swapping scenes");
    // Call UNLOAD_SCENE hooks on old scene
    if (context->m_current.scene) {
        modules::hooks::unload_scene(context->m_modules_ctx, context->m_current.scene, context->m_scenes[context->m_current.scene]);
    }

    // Set current scene and scripts
    context->m_current.scene = context->m_pending.scene;
    context->m_current.scripts = context->m_pending.scripts;

    // Invalidate pending
    context->m_pending.scene = 0;
    context->m_pending.scripts = million::resources::Handle::invalid();

    // Swap newly loaded scene into foreground
    context->m_registries.swap();
    // Copy entities marked as "global" from background to foreground
    context->m_registries.copyGlobals();
    // Clear the background registry
    context->m_registries.background().clear();
    // Set context variables
    context->m_registries.foreground().runtime.ctx().emplace<million::api::Runtime>(context->m_context_data);

    // Call LOAD_SCENE hooks on new scene
    // TODO: Create a scene API object to pass in? What can it do?
    if (context->m_current.scene) {
        modules::hooks::unload_scene(context->m_modules_ctx, context->m_current.scene, context->m_scenes[context->m_current.scene]);
        context->m_stream.emit<events::scene::Activated>([context](auto& scene){
            scene.id = context->m_current.scene;
        });
    }
}