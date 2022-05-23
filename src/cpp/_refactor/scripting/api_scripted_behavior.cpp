#include "scripting.hpp"
#include "context.hpp"

#include "_refactor/world/world.hpp"

extern "C" BehaviorIterator* setup_scripted_behavior_iterator (scripting::Context* context)
{
    EASY_FUNCTION(scripting::COLOR(3));
    const auto& registry = world::registry(context->m_world_ctx);
    const auto& storage = registry.storage<components::core::ScriptedBehavior>();
    BehaviorIterator::const_iterable iter = storage.each();
    context->m_behavior_iterator = {iter.begin(), iter.end()};
    return &context->m_behavior_iterator;
}

extern "C" std::uint32_t get_next_scripted_behavior (scripting::Context* context, BehaviorIterator* iter, const components::core::ScriptedBehavior** behavior)
{
    EASY_FUNCTION(scripting::COLOR(3));
    if (iter->next != iter->end) {
        const auto&& [entity, sb] = *iter->next;
        ++iter->next;
        *behavior = &sb;
        return magic_enum::enum_integer(entity);
    } else {
        *behavior = nullptr;
        return magic_enum::enum_integer(entt::entity{entt::null});
    }
}
