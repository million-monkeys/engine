#pragma once

#include <monkeys.hpp>
#include <million/engine.hpp>

class SetupAPI : public million::api::EngineSetup
{
public:
    void registerGameHandler (entt::hashed_string state, entt::hashed_string::hash_type events, million::GameHandler handler) final;
    void registerSceneHandler (entt::hashed_string scene, entt::hashed_string::hash_type events, million::SceneHandler handler) final;
    void readBinaryFile (const std::string& filename, std::string& buffer) const final;
    million::resources::Handle loadResource (entt::hashed_string, const std::string&, entt::hashed_string::hash_type) final;
    entt::registry& registry (million::Registry) final;
    entt::organizer& organizer (million::SystemStage) final;
    million::events::Publisher& publisher() final;
    million::events::Stream& commandStream() final;
    million::events::Stream& createStream (entt::hashed_string, million::StreamWriters=million::StreamWriters::Single, std::uint32_t=0) final;
};