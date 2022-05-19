#pragma once

#include <game.hpp>

namespace events {

    struct Context;

    Context* init ();
    void term (Context*);

    void pump (Context*);

    million::events::Stream& createStream (events::Context* context, entt::hashed_string stream_name, million::StreamWriters writers=million::StreamWriters::Single);
    void createEngineStream (Context* context, entt::hashed_string stream_name, million::StreamWriters writers);

    million::events::Stream& commandStream(Context*);
    million::events::Stream* engineStream (Context* context, entt::hashed_string stream_name);

    const million::events::EventIterable events (Context* context, entt::hashed_string stream_name);
    const million::events::EventIterable events (Context* context, entt::hashed_string::hash_type stream_hash);

}
