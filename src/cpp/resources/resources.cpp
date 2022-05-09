
#include "resources.hpp"
#include "builtintypes.hpp"

#include "core/engine.hpp"

#include <thread>
#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>

class ResourceStorage {
public:
    ResourceStorage (std::uint32_t id, million::api::resources::Loader* loader, bool managed) : m_resource_id(id), m_loader(loader), m_managed(managed) {}
    ResourceStorage (ResourceStorage&& other) :
        m_resource_id(other.m_resource_id),
        m_loader(other.m_loader),
        m_managed(other.m_managed)
    {
        other.m_loader = nullptr;
    }
    ~ResourceStorage()
    {
        if (m_managed && m_loader != nullptr) {
            delete m_loader;
        }
    }

    million::resources::Handle enqueue (const std::string& filename, entt::hashed_string::hash_type name);
    
private:
    std::atomic_int32_t m_idx = 0;
    const std::uint32_t m_resource_id;
    million::api::resources::Loader* m_loader;
    const bool m_managed;
};

phmap::flat_hash_map<entt::hashed_string::hash_type, ResourceStorage, helpers::Identity> g_resource_loaders;

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
    resources::install<resources::types::EntityScripts>(engine);
    resources::install<resources::types::GameScripts>(engine);
    resources::install<resources::types::SceneScripts>(engine);
    resources::install<resources::types::SceneEntities>(engine);
    g_loader_thread = std::thread(loaderThread);
}

void resources::poll ()
{
    std::size_t count;
    do {
        count = g_done_queue.try_dequeue_bulk(g_done_items, MAX_DONE_ITEMS);
        for (std::size_t i = 0; i != count; ++i) {
            auto& loaded = g_stream->emit<events::resources::Loaded>();
            auto& item = g_done_items[i];
            loaded.type = item.type;
            loaded.name = item.name;
            loaded.handle = item.handle;
        }
    } while (count > 0);

}

void resources::install (million::api::resources::Loader* loader, bool managed)
{
    const auto type = loader->name();
    std::uint32_t index = g_resource_loaders.size() + 1;
    spdlog::debug("[resources] Installing {} ({:x})", type.data(), index);
    g_resource_loaders.emplace(type.value(), ResourceStorage{index, loader, managed});
}

million::resources::Handle resources::load (entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    EASY_FUNCTION(profiler::colors::Teal100);
    auto it = g_resource_loaders.find(type.value());
    if (it != g_resource_loaders.end()) {
        return it->second.enqueue(filename, name);
    } else {
        spdlog::warn("[resources] Could not load '{}' from '{}', resource type does not exist", type.data(), filename);
        return million::resources::Handle::invalid();
    }
}

void resources::term ()
{
    g_work_queue.enqueue(WorkItem::POISON_PILL);
    g_loader_thread.join();
    g_resource_loaders.clear();
}

million::resources::Handle ResourceStorage::enqueue (const std::string& filename, entt::hashed_string::hash_type name)
{
    std::uint32_t id;
    if (m_loader->cached(filename, &id)) {
        return million::resources::Handle::make(m_resource_id, id);
    }
    EASY_FUNCTION(profiler::colors::Teal300);
    million::resources::Handle handle = million::resources::Handle::make(m_resource_id, m_idx.fetch_add(1));
    g_work_queue.enqueue({m_loader, filename, handle, name});
    return handle;
}