#include "resources.hpp"
#include "context.hpp"

#include "events/events.hpp"

void loaderThread (resources::Context* context)
{
    EASY_THREAD("Resource Loader");
    SPDLOG_DEBUG("[resources] Starting resource loader thread");
    WorkItem item = WorkItem::POISON_PILL;
    do {
        context->m_work_queue.wait_dequeue(item);
        if (item == WorkItem::POISON_PILL) {
            break;
        }

        EASY_BLOCK("Loading Resource", resources::COLOR(1));

        SPDLOG_DEBUG("[resources] Loading {}/{:#x} from file: {}", item.loader->name().data(), item.handle.handle, item.filename);
        auto loaded = item.loader->load(item.handle, item.filename);
        if (!loaded) {
            spdlog::warn("[resources] Could not load {}/{:#x} from file: {}", item.loader->name().data(), item.handle.handle, item.filename);
            continue;
        }
        SPDLOG_DEBUG("[resources] Loaded {}/{:#x}", item.loader->name().data(), item.handle.handle);
        context->m_done_queue.enqueue({item.loader->name(), item.handle, item.name});

    } while (true);
    SPDLOG_DEBUG("[resources] Terminating resource loader thread");
}

resources::Context* resources::init (events::Context* events_ctx)
{
    EASY_BLOCK("resources::init", resources::COLOR(1));
    SPDLOG_DEBUG("[resources] Init");
    auto context = new resources::Context{
        events::createStream(events_ctx, "resources"_hs),
    };
    context->m_loader_thread = std::thread(loaderThread, context);
    return context;
}

void resources::term (resources::Context* context)
{
    EASY_BLOCK("resources::term", resources::COLOR(1));
    SPDLOG_DEBUG("[resources] Term");
    context->m_work_queue.enqueue(WorkItem::POISON_PILL);
    context->m_loader_thread.join();
    context->m_resource_loaders.clear();
    delete context;
}
