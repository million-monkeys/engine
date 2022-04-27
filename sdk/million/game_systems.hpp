#pragma once

#include <entt/entity/organizer.hpp>
#include <functional>

namespace million::events {
    class OutputStream;
}

namespace million::systems {
    class GameSystem
    {
    public:
        virtual ~GameSystem() {}
    };
    // using CreateFunction = entt::delegate<GameSystem*(million::events::OutputStream&, entt::organizer&, entt::organizer&)>;
}
