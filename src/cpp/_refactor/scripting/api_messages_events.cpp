#include "scripting.hpp"
#include "context.hpp"
#include "_refactor/messages/messages.hpp"
#include "_refactor/events/events.hpp"

#include "_refactor/memory/event_pools.hpp"

void set_active_stream (scripting::Context* context, entt::hashed_string stream_name)
{
    context->m_active_stream = events::engineStream(context->m_events_ctx, stream_name);
}

extern "C" std::uint32_t get_category (scripting::Context* context, const char* category_name)
{
    // return g_engine->categoryBitflag(entt::hashed_string::value(category_name));
    return 0;
}

extern "C" void* allocate_message (scripting::Context* context, const char* message_name, std::uint32_t target, std::uint16_t flags, std::uint16_t size)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::hashed_string::hash_type message_type = entt::hashed_string::value(message_name);
    return messages::publisher(context->m_messages_ctx).push(message_type, target, flags, size);
}

extern "C" std::uint32_t get_messages (scripting::Context* context, const char** buffer)
{
    EASY_FUNCTION(scripting::COLOR(3));
    messages::pump(context->m_messages_ctx);
    auto [begin, end] = messages::messages(context->m_messages_ctx);
    *buffer = reinterpret_cast<const char*>(begin);
    return end - begin;
}

extern "C" std::uint32_t get_stream_events (scripting::Context* context, std::uint32_t stream_name, const char** buffer)
{
    EASY_FUNCTION(scripting::COLOR(3));
    auto iterable = events::events(context->m_events_ctx, stream_name);
    auto& first = *iterable.begin();
    *buffer = reinterpret_cast<const char*>(&first);
    return iterable.size();
}

extern "C" void* allocate_command (scripting::Context* context, const char* event_name, std::uint8_t size)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::hashed_string::hash_type event_type = entt::hashed_string::value(event_name);
    return memory::RawAccessWrapper(events::commandStream(context->m_events_ctx)).push(event_type, size);
}

extern "C" void* allocate_event (scripting::Context* context, const char* event_name, std::uint8_t size)
{
    EASY_FUNCTION(scripting::COLOR(3));
    entt::hashed_string::hash_type event_type = entt::hashed_string::value(event_name);
    return memory::RawAccessWrapper(*context->m_active_stream).push(event_type, size);
}
