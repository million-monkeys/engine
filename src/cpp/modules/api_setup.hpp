#pragma once

#include <monkeys.hpp>
#include <million/engine.hpp>

#include "game/game.hpp"
#include "world/world.hpp"
#include "resources/resources.hpp"
#include "scheduler/scheduler.hpp"
#include "messages/messages.hpp"
#include "events/events.hpp"

class SetupAPI : public million::api::EngineSetup
{
public:
    SetupAPI (world::Context* world_ctx, game::Context* game_ctx, resources::Context* resources_ctx, scheduler::Context* scheduler_ctx, messages::Context* messages_ctx, events::Context* events_ctx) : m_world_ctx(world_ctx), m_game_ctx(game_ctx), m_resources_ctx(resources_ctx), m_scheduler_ctx(scheduler_ctx), m_messages_ctx(messages_ctx), m_events_ctx(events_ctx) {}
    virtual ~SetupAPI () {}

    void registerGameHandler (entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler) final
    {
        game::registerHandler(m_game_ctx, state, events, handler);
    }

    void registerSceneHandler (entt::hashed_string scene, entt::hashed_string::hash_type events, million::SceneHandler handler) final
    {
        world::registerHandler(m_world_ctx, scene, events, handler);
    }

    void readBinaryFile (const std::string& filename, std::string& buffer) const final
    {

    }

    million::resources::Handle findResource (entt::hashed_string::hash_type name) const final
    {
        return resources::find(m_resources_ctx, name);
    }

    million::resources::Handle loadResource (entt::hashed_string type, const std::string& file, entt::hashed_string::hash_type name) final
    {
        return resources::load(m_resources_ctx, type, file, name);
    }

    entt::registry& registry () final
    {
        return world::registry(m_world_ctx);
    }

    entt::organizer& organizer (million::SystemStage stage) final
    {
        return scheduler::organizer(m_scheduler_ctx, stage);
    }

    million::events::Publisher& publisher() final
    {
        return messages::publisher(m_messages_ctx);
    }

    million::events::Stream& commandStream() final
    {
        return events::commandStream(m_events_ctx);
    }

    million::events::Stream& createStream (entt::hashed_string name, million::StreamWriters writers) final
    {
        return events::createStream(m_events_ctx, name, writers);
    }

private:
    world::Context* m_world_ctx;
    game::Context* m_game_ctx;
    resources::Context* m_resources_ctx;
    scheduler::Context* m_scheduler_ctx;
    messages::Context* m_messages_ctx;
    events::Context* m_events_ctx;
};