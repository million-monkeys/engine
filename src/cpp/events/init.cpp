#include "events.hpp"
#include "context.hpp"

events::Context::Context ()
    : m_commands(events::createStream(this, "commands"_hs, million::StreamWriters::Multi))
{
}

events::Context* events::init ()
{
    return new events::Context;
}

void events::term (events::Context* context)
{
    delete context;
}