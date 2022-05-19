#pragma once

#include <game.hpp>
#include <atomic>
#include <thread>
#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>

class ResourceStorage {
public:
    ResourceStorage (std::uint32_t id, million::api::resources::Loader* loader, bool managed, resources::Context* context) : m_resource_id(id), m_loader(loader), m_managed(managed), m_context(context) {}
    ResourceStorage (ResourceStorage&& other) :
        m_resource_id(other.m_resource_id),
        m_loader(other.m_loader),
        m_managed(other.m_managed),
        m_context(other.m_context)
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
    resources::Context* m_context;
};

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

namespace resources {
    struct Context {
        helpers::hashed_string_flat_map<million::resources::Handle> m_named_resources;
        helpers::hashed_string_flat_map<ResourceStorage> m_resource_loaders;
        million::events::Stream* m_stream;
        moodycamel::BlockingConcurrentQueue<WorkItem> m_work_queue;
        moodycamel::ConcurrentQueue<DoneItem> m_done_queue;
        std::thread m_loader_thread;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Blue900,
            profiler::colors::Blue700,
            profiler::colors::Blue500,
            profiler::colors::Blue300,
            profiler::colors::Blue100,
        };
        return colors[idx];
    }
}
