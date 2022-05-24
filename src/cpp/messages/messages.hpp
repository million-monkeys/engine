#pragma once

#include <monkeys.hpp>

namespace messages {
    Context* init ();
    void term (Context*);

    void pump (messages::Context* context);

    million::events::Publisher& publisher (Context* context);
    const std::pair<std::byte*, std::byte*> messages (Context* context);
}
