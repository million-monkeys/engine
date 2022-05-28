#include "game.hpp"
#include "context.hpp"

#include "config/config.hpp"
#include "events/events.hpp"
#include "world/world.hpp"
#include "scripting/scripting.hpp"
#include "modules/modules.hpp"

#include "loaders/game_scripts.hpp"

game::Context* game::init (events::Context* events_ctx, messages::Context* messages_ctx, world::Context* world_ctx, scripting::Context* scripting_ctx, resources::Context* resources_ctx, modules::Context* modules_ctx)
{
    EASY_BLOCK("game::init", game::COLOR(1));
    SPDLOG_DEBUG("[game] Init");
    auto context = new game::Context{
        events::createStream(events_ctx, "game"_hs),
        events::commandStream(events_ctx),
    };
    context->m_events_ctx = events_ctx;
    context->m_messages_ctx = messages_ctx;
    context->m_world_ctx = world_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_resources_ctx = resources_ctx;
    context->m_modules_ctx = modules_ctx;

    resources::install<loaders::GameScripts>(resources_ctx, context);

    return context;
}

void game::term (game::Context* context)
{
    EASY_BLOCK("game::term", game::COLOR(1));
    SPDLOG_DEBUG("[game] Term");
    delete context;
}

void game::setScheduler (game::Context* context, scheduler::Context* scheduler_ctx)
{
    context->m_scheduler_ctx = scheduler_ctx;
}

std::optional<scheduler::SystemStatus> game::setup (game::Context* context)
{
    EASY_BLOCK("game::setup", game::COLOR(1));
    SPDLOG_DEBUG("[game] Setup");
    // Read game configuration
    helpers::hashed_string_flat_map<std::string> game_scripts;
    std::vector<entt::hashed_string::hash_type> entity_categories;
    if (!config::readGameConfig(context->m_scripting_ctx, game_scripts, entity_categories, context->m_modules_ctx)) {
        return std::nullopt;
    }
    SPDLOG_TRACE("[game] Game config read");

    // Load user mods
    // const std::string& user_mods = entt::monostate<"game/user-mods"_hs>();
    // TomlTable mods_table;
    // modules::load(context->m_modules_ctx, nullptr, user_mods, mods_table);

    // Call module hooks
    modules::hooks::game_setup(context->m_modules_ctx);

    // Add entity categories
    world::setEntityCategories(context->m_world_ctx, entity_categories);

    // Set up game scenes
    const std::string& scene_path = entt::monostate<"scenes/path"_hs>();
    world::loadSceneList(context->m_world_ctx, scene_path);
    SPDLOG_TRACE("[game] Scene list loaded");

    // Make sure game-specific events are declared in Lua
    scripting::load(context->m_scripting_ctx, entt::monostate<"game/script-events"_hs>());
    SPDLOG_TRACE("[game] Game event definitions loaded");

    // Create game task graph
    scheduler::createTaskGraph(context->m_scheduler_ctx);
    SPDLOG_TRACE("[game] Task graph created");

    // Set up game state
    const std::string& start_state_str = entt::monostate<"game/initial-state"_hs>();
    auto start_state = entt::hashed_string{start_state_str.c_str()};
    game::setState(context, start_state);
    SPDLOG_TRACE("[game] Game state set");

    scheduler::SystemStatus status =  scheduler::SystemStatus::Running;

    // Load game event handler scripts
    for (const auto& [game_state, script_file] :  game_scripts) {
        if (game_state == start_state) {
            status = scheduler::SystemStatus::Loading;
        }
        resources::load(context->m_resources_ctx, "game-script"_hs, script_file, game_state);
    }
    SPDLOG_TRACE("[game] Resources queued for loading");

    // load initial scene
    context->m_commands.emit<commands::scene::Load>([](auto& load){
        const std::string initial_scene = entt::monostate<"scenes/initial"_hs>();
        load.scene_id = entt::hashed_string::value(initial_scene.c_str());
        load.auto_swap = true;
    });
    SPDLOG_TRACE("[game] Initial scene loading requested");

    // Make events emitted during load visible on first frame
    events::pump(context->m_events_ctx);

    SPDLOG_TRACE("[game] Game setup complete");
    return status;
}