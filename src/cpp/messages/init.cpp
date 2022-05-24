#include "messages.hpp"
#include "context.hpp"

// TODO: Move to utils
// Taskflow workers = number of hardware threads - 1, unless there is only one hardware thread
int get_num_workers () {
    auto max_workers = std::thread::hardware_concurrency();
    return max_workers > 1 ? max_workers - 1 : max_workers;
}

int get_global_event_pool_size () {
    const std::uint32_t pool_size = entt::monostate<"memory/events/pool-size"_hs>();
    return pool_size * get_num_workers();
}

messages::Context::Context () :
    m_message_pool(get_global_event_pool_size())
{

}

messages::Context* messages::init ()
{
    return new messages::Context{};
}

void messages::term (messages::Context* context)
{
    delete context;
}