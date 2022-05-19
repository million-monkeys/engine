#include "messages.hpp"
#include "context.hpp"

#include "core/engine.hpp"


#include <iterator>
#include <mutex>

std::mutex g_pool_mutex;

thread_local memory::MessagePublisher<memory::MessagePool> g_message_publisher;

void messages::pump (messages::Context* context)
{
    EASY_FUNCTION(messages::COLOR(2));
    context->m_message_pool.reset();
    // Copy thread local events into global pool and reset thread local pools
    for (auto pool : context->m_message_pools) {
        pool->copyInto(context->m_message_pool);
        pool->reset();
    }
}

million::events::Publisher& messages::publisher (messages::Context* context)
{
    if EXPECT_NOT_TAKEN(! g_message_publisher.valid()) {
        EASY_FUNCTION(messages::COLOR(3));
        // Lazy initialisation is unfortunately the only way we can initialise thread_local variables after config is read and subsystem contexts are available
        // TODO: investigate making the thread locals register themselves with an intitializer mechanism that will initialize them after config and subsystems are available
        const std::uint32_t message_pool_size = entt::monostate<"memory/events/pool-size"_hs>();

        // Only one thread can access event pools list at once
        std::lock_guard<std::mutex> guard(g_pool_mutex);
        auto message_pool = new memory::MessagePool(message_pool_size);
        context->m_message_pools.push_back(message_pool); // Keep track of this pool so that we can gather the events into a global pool at the end of each frame
        g_message_publisher = memory::MessagePublisher<memory::MessagePool>(message_pool);
    }
    return g_message_publisher;
}

const std::pair<std::byte*, std::byte*> messages::messages (messages::Context* context)
{
    return std::make_pair(context->m_message_pool.begin(), context->m_message_pool.end());
}
