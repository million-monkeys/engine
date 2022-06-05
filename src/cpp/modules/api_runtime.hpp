#include <monkeys.hpp>
#include <million/engine.hpp>

#include "world/world.hpp"
#include "resources/resources.hpp"
#include "messages/messages.hpp"
#include "events/events.hpp"

class RuntimeAPI : public million::api::EngineRuntime
{
public:
    RuntimeAPI (world::Context* world_ctx, resources::Context* resources_ctx, messages::Context* messages_ctx, events::Context* events_ctx) : m_world_ctx(world_ctx), m_resources_ctx(resources_ctx), m_messages_ctx(messages_ctx), m_events_ctx(events_ctx) {}
    virtual ~RuntimeAPI () {}

    entt::entity findEntity (entt::hashed_string name) const final
    {
        return world::findEntity(m_world_ctx, name);
    }

    const std::string& findEntityName (const components::core::Named& named) const final
    {
        return world::findEntityName(m_world_ctx, named);
    }

    const std::string& findEntityName (entt::entity entity) const final
    {
        return world::findEntityName(m_world_ctx, entity);
    }

    entt::entity loadEntity (entt::hashed_string prototype_id) final
    {
        return world::loadEntity(m_world_ctx, prototype_id);
    }

    void mergeEntity (entt::entity entity, entt::hashed_string prototype_id, bool overwrite_components) final
    {
        world::mergeEntity(m_world_ctx, entity, prototype_id, overwrite_components);
    }

    million::resources::Handle findResource (entt::hashed_string::hash_type name) const final
    {
        return resources::find(m_resources_ctx, name);
    }

    million::resources::Handle loadResource (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name) final
    {
        return resources::load(m_resources_ctx, type, filename, name);
    }

    million::events::Publisher& publisher() final
    {
        return messages::publisher(m_messages_ctx);
    }

    million::events::Stream& commandStream() final
    {
        return events::commandStream(m_events_ctx);
    }

    const million::events::EventIterable events (entt::hashed_string stream_name) const final
    {
        return events::events(m_events_ctx, stream_name);
    }

private:
    world::Context* m_world_ctx;
    resources::Context* m_resources_ctx;
    messages::Context* m_messages_ctx;
    events::Context* m_events_ctx;
};