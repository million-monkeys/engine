#include "resources.hpp"
#include "context.hpp"

constexpr int MAX_DONE_ITEMS = 10;

void resources::poll (resources::Context* context)
{
    EASY_FUNCTION(resources::COLOR(1));
    DoneItem done_items[MAX_DONE_ITEMS];
    std::size_t count;
    do {
        count = context->m_done_queue.try_dequeue_bulk(done_items, MAX_DONE_ITEMS);
        for (std::size_t i = 0; i != count; ++i) {
            auto& loaded = context->m_stream.emit<events::resources::Loaded>();
            auto& item = done_items[i];
            loaded.type = item.type;
            loaded.name = item.name;
            loaded.handle = item.handle;
        }
    } while (count > 0);
}

void resources::install (resources::Context* context, million::api::resources::Loader* loader, bool managed)
{
    EASY_FUNCTION(resources::COLOR(1));
    const auto type = loader->name();
    std::uint32_t index = context->m_resource_loaders.size() + 1;
    spdlog::debug("[resources] Installing {} ({:x})", type.data(), index);
    context->m_resource_loaders.emplace(type.value(), ResourceStorage{index, loader, managed, context});
}

million::resources::Handle loadResource (resources::Context* context, entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    EASY_FUNCTION(resources::COLOR(2));
    auto it = context->m_resource_loaders.find(type.value());
    if (it != context->m_resource_loaders.end()) {
        return it->second.enqueue(filename, name);
    } else {
        spdlog::warn("[resources] Could not load '{}' from '{}', resource type does not exist", type.data(), filename);
        return million::resources::Handle::invalid();
    }
}

million::resources::Handle resources::load (resources::Context* context, entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    EASY_FUNCTION(resources::COLOR(1));
    auto it = context->m_resource_loaders.find(type.value());
    if (it == context->m_resource_loaders.end()) {
        spdlog::warn("[resources] Could not load '{}' from '{}', resource type does not exist", type.data(), filename);
        return million::resources::Handle::invalid();
    }
    auto handle = it->second.enqueue(filename, name);
    if (name != 0) {
        resources::bindToName(context, handle, name);
    }
    return handle;
}

million::resources::Handle resources::find (resources::Context* context, entt::hashed_string::hash_type name)
{
    EASY_FUNCTION(resources::COLOR(1));
    auto it = context->m_named_resources.find(name);
    if (it != context->m_named_resources.end()) {
        spdlog::debug("Resource with name {:#x} found: {:#x}", name, it->second.handle);
        return it->second;
    }
    spdlog::debug("Resource with name {:#x} not found", name);
    return million::resources::Handle::invalid();
}

void resources::bindToName (resources::Context* context, million::resources::Handle handle, entt::hashed_string::hash_type name)
{
    EASY_FUNCTION(resources::COLOR(4));
    context->m_named_resources[name] = handle;
    spdlog::debug("Bound resource {:#x} to name {:#x}", handle.handle, name);
}

million::resources::Handle ResourceStorage::enqueue (const std::string& filename, entt::hashed_string::hash_type name)
{
    std::uint32_t id;
    if (m_loader->cached(filename, &id)) {
        return million::resources::Handle::make(m_resource_id, id);
    }
    EASY_FUNCTION(profiler::colors::Teal300);
    million::resources::Handle handle = million::resources::Handle::make(m_resource_id, m_idx.fetch_add(1));
    m_context->m_work_queue.enqueue({m_loader, filename, handle, name});
    return handle;
}
