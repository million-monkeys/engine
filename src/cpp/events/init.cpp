#include "events.hpp"
#include "context.hpp"

events::Context::Context ()
    : m_commands(events::createStream(this, "commands"_hs, million::StreamWriters::Multi))
{
}

events::Context* events::init ()
{
    EASY_BLOCK("events::init", events::COLOR(1));
    SPDLOG_DEBUG("[events] Init");
    return new events::Context;
}

void events::term (events::Context* context)
{
    EASY_BLOCK("events::term", events::COLOR(1));
    SPDLOG_DEBUG("[events] Term");
    delete context;
}