#pragma once

#include <game.hpp>

namespace messages {
    struct Context;

    Context* init ();
    void term (Context*);

    void pump (messages::Context* context);

    million::events::Publisher& publisher (Context* context);
    const std::pair<std::byte*, std::byte*> messages (Context* context);
}
