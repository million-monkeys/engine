#include "world.hpp"
#include "context.hpp"

void Registries::copyRegistry (const entt::registry& from, entt::registry& to)
{
    EASY_FUNCTION(profiler::colors::RichYellow);
    from.each([&from,&to](const auto source_entity) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            if(source_storage.contains(source_entity)) {
                auto it = to.storage(id);
                if (it != to.storage().end()) {
                    it->second.emplace(destination_entity, source_storage.get(source_entity));
                }
            }
        }
    });
}

void Registries::copyGlobals ()
{
    // Entity names will be automatically copied by on_construct
    EASY_FUNCTION(profiler::colors::RichYellow);
    auto& from = background().runtime;
    auto& to = foreground().runtime;
    for (const auto&& [source_entity] : from.storage<components::core::Global>().each()) {
        auto destination_entity = to.create();
        for(auto [id, source_storage]: from.storage()) {
            if(source_storage.contains(source_entity)) {
                auto it = to.storage(id);
                if (it != to.storage().end()) {
                    it->second.emplace(destination_entity, source_storage.get(source_entity));
                }
            }
        }
    }
}

entt::registry& world::registry (world::Context* context)
{
    return context->m_registries.foreground().runtime;
}
