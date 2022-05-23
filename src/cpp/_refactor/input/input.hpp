#pragma once

#include <monkeys.hpp>

namespace input {
    Context* init (events::Context* events_ctx);
    void term (Context* context);

    void poll (input::Context* context);
}