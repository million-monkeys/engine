
#include "resources.hpp"
#include "builtintypes.hpp"

#include "core/engine.hpp"

#include <thread>
#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>

phmap::flat_hash_map<entt::hashed_string::hash_type, million::api::resources::Loader*, helpers::Identity> g_resource_loaders;

struct WorkItem {
    million::api::resources::Loader* loader;
    std::string filename;
    million::resources::Handle handle;
    entt::hashed_string::hash_type name;

    bool operator==(const WorkItem& other) const {
        return loader == other.loader && handle.handle == other.handle.handle && filename == other.filename;
    }

    static const WorkItem POISON_PILL;
};
const WorkItem WorkItem::POISON_PILL = WorkItem{nullptr, "", million::resources::Handle::invalid()};

struct DoneItem {
    entt::hashed_string::hash_type type;
    million::resources::Handle handle;
    entt::hashed_string::hash_type name;
};

moodycamel::BlockingConcurrentQueue<WorkItem> g_work_queue;
moodycamel::ConcurrentQueue<DoneItem> g_done_queue;

constexpr int MAX_DONE_ITEMS = 10;
DoneItem g_done_items[MAX_DONE_ITEMS];

void loaderThread ()
{
    spdlog::debug("[resources] Starting resource loader thread");
    WorkItem item = WorkItem::POISON_PILL;
    do {
        g_work_queue.wait_dequeue(item);
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
        g_done_queue.enqueue({item.loader->name(), item.handle, item.name});

    } while (true);
    spdlog::debug("[resources] Terminating resource loader thread");
}

std::thread g_loader_thread;
million::events::Stream* g_stream = nullptr;

void resources::init (core::Engine* engine)
{
    g_stream = &engine->createStream("resources"_hs);
    resources::install<resources::types::ScriptedEvents>();
    resources::install<resources::types::SceneScripts>();
    resources::install<resources::types::SceneEntities>();
    g_loader_thread = std::thread(loaderThread);
}

void resources::poll ()
{
    std::size_t count;
    do {
        count = g_done_queue.try_dequeue_bulk(g_done_items, MAX_DONE_ITEMS);
        for (std::size_t i = 0; i != count; ++i) {
            auto& loaded = g_stream->emit<events::engine::ResourceLoaded>();
            auto& item = g_done_items[i];
            loaded.name = item.name;
            loaded.handle = item.handle;
        }
    } while (count > 0);

}

void resources::install (entt::id_type id, million::api::resources::Loader* loader)
{
    const auto type = loader->name();
    spdlog::debug("[resources] Installing {}", type.data());
    g_resource_loaders[type.value()] = loader;
}

std::uint32_t g_idx = 0;

class ResourceStorage {
public:
    ResourceStorage (std::uint32_t id) : m_type_id(id) {}

    million::resources::Handle allocate ()
    {
        return million::resources::Handle::make(m_type_id, m_idx++);
    }
    
private:
    const std::uint32_t m_type_id;
    std::uint32_t m_idx = 0;
};

million::resources::Handle resources::load (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    auto it = g_resource_loaders.find(type.value());
    if (it != g_resource_loaders.end()) {
        million::resources::Handle handle = million::resources::Handle::make(1, g_idx++);
        g_work_queue.enqueue({it->second, filename, handle, name});
        return handle;
    } else {
        spdlog::warn("[resources] Could not load '{}' from '{}', resource type does not exist", type.data(), filename);
        return million::resources::Handle::invalid();
    }
}

void resources::term ()
{
    g_work_queue.enqueue(WorkItem::POISON_PILL);
    g_loader_thread.join();

    delete g_resource_loaders[resources::types::ScriptedEvents::Name];

    g_resource_loaders.clear();
}
