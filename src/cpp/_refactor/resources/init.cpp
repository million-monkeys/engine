#include "resources.hpp"
#include "context.hpp"

#include "_refactor/events/events.hpp"

void loaderThread (resources::Context* context)
{
    spdlog::debug("[resources] Starting resource loader thread");
    WorkItem item = WorkItem::POISON_PILL;
    do {
        context->m_work_queue.wait_dequeue(item);
        if (item == WorkItem::POISON_PILL) {
            break;
        }

        EASY_BLOCK("Loading Resource", profiler::colors::Teal200);

        SPDLOG_DEBUG("[resources] Loading {}/{:#x} from file: {}", item.loader->name().data(), item.handle.handle, item.filename);
        auto loaded = item.loader->load(item.handle, item.filename);
        if (!loaded) {
            spdlog::warn("[resources] Could not load {}/{:#x} from file: {}", item.loader->name().data(), item.handle.handle, item.filename);
            continue;
        }
        SPDLOG_DEBUG("[resources] Loaded {}/{:#x}", item.loader->name().data(), item.handle.handle);
        context->m_done_queue.enqueue({item.loader->name(), item.handle, item.name});

    } while (true);
    spdlog::debug("[resources] Terminating resource loader thread");
}

resources::Context* resources::init (events::Context* events_ctx)
{
    auto context = new resources::Context{
        events::createStream(events_ctx, "resources"_hs),
    };
    context->m_loader_thread = std::thread(loaderThread, context);
    return context;
}

void resources::term (resources::Context* context)
{
    context->m_work_queue.enqueue(WorkItem::POISON_PILL);
    context->m_loader_thread.join();
    context->m_resource_loaders.clear();
    delete context;
}
