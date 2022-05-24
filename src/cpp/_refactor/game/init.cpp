#include "game.hpp"
#include "context.hpp"

#include "_refactor/config/config.hpp"
#include "_refactor/events/events.hpp"
#include "_refactor/world/world.hpp"
#include "_refactor/scripting/scripting.hpp"

#include "loaders/game_scripts.hpp"

game::Context* game::init (events::Context* events_ctx, messages::Context* messages_ctx, world::Context* world_ctx, scripting::Context* scripting_ctx, resources::Context* resources_ctx)
{
    auto context = new game::Context{
        events::createStream(events_ctx, "game"_hs),
        events::commandStream(events_ctx),
    };
    context->m_events_ctx = events_ctx;
    context->m_messages_ctx = messages_ctx;
    context->m_world_ctx = world_ctx;
    context->m_scripting_ctx = scripting_ctx;
    context->m_resources_ctx = resources_ctx;

    resources::install<loaders::GameScripts>(resources_ctx, context);

    return context;
}

void game::term (game::Context* context)
{
    delete context;
}

void game::setScheduler (game::Context* context, scheduler::Context* scheduler_ctx)
{
    context->m_scheduler_ctx = scheduler_ctx;
}

std::optional<scheduler::SystemStatus> game::setup (game::Context* context)
{
    EASY_FUNCTION(profiler::colors::Pink50);
    // Read game configuration
    helpers::hashed_string_flat_map<std::string> game_scripts;
    std::vector<entt::hashed_string::hash_type> entity_categories;
    if (!config::readGameConfig(game_scripts, entity_categories)) {
        return std::nullopt;
    }

    // Add entity categories
    world::setEntityCategories(context->m_world_ctx, entity_categories);

    // Set up game scenes
    const std::string& scene_path = entt::monostate<"scenes/path"_hs>();
    world::loadSceneList(context->m_world_ctx, scene_path);

    // Make sure game-specific events are declared in Lua
    scripting::load(context->m_scripting_ctx, entt::monostate<"game/script-events"_hs>());

    // load initial scene
    context->m_commands.emit<commands::scene::Load>([](auto& load){
        const std::string initial_scene = entt::monostate<"scenes/initial"_hs>();
        load.scene_id = entt::hashed_string::value(initial_scene.c_str());
        load.auto_swap = true;
    });

    // Set up game state
    const std::string& start_state_str = entt::monostate<"game/initial-state"_hs>();
    auto start_state = entt::hashed_string{start_state_str.c_str()};
    game::setState(context, start_state);

    scheduler::SystemStatus status =  scheduler::SystemStatus::Running;

    // Load game event handler scripts
    for (const auto& [game_state, script_file] :  game_scripts) {
        if (game_state == start_state) {
            status = scheduler::SystemStatus::Loading;
        }
        resources::load(context->m_resources_ctx, "game-script"_hs, script_file, game_state);
    }

    // Make events emitted during load visible on first frame
    events::pump(context->m_events_ctx);

    return status;
}