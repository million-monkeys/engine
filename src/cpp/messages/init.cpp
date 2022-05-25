#include "messages.hpp"
#include "context.hpp"

int get_global_event_pool_size () {
    const std::uint32_t pool_size = entt::monostate<"memory/events/pool-size"_hs>();
    return pool_size * std::thread::hardware_concurrency();
}

messages::Context::Context () :
    m_message_pool(get_global_event_pool_size())
{

}

messages::Context* messages::init ()
{
    EASY_BLOCK("messages::init", messages::COLOR(1));
    SPDLOG_DEBUG("[messages] Init");
    return new messages::Context{};
}

void messages::term (messages::Context* context)
{
    EASY_BLOCK("messages::term", messages::COLOR(1));
    SPDLOG_DEBUG("[messages] Term");
    delete context;
}