#include "resources.hpp"
#include "context.hpp"

const WorkItem WorkItem::POISON_PILL = WorkItem{nullptr, "", million::resources::Handle::invalid()};
constexpr int MAX_DONE_ITEMS = 10;

void resources::poll (resources::Context* context)
{
    EASY_BLOCK("resources::poll", resources::COLOR(1));
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
    EASY_BLOCK("resources::install", resources::COLOR(1));
    const auto type = loader->name();
    std::uint32_t index = context->m_resource_loaders.size() + 1;
    SPDLOG_DEBUG("[resources] Installing {} ({}, 0x{:x})", type.data(), index, index << 22);
    context->m_resource_loaders.emplace(type.value(), ResourceStorage{index, loader, managed, context});
}

million::resources::Handle resources::load (resources::Context* context, entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    EASY_BLOCK("resources::load", resources::COLOR(1));
    auto it = context->m_resource_loaders.find(type.value());
    if (it == context->m_resource_loaders.end()) {
        spdlog::warn("[resources] Could not load '{}' from '{}', resource type does not exist", type.data(), filename);
        return million::resources::Handle::invalid();
    }
    auto handle = it->second.enqueue(filename, name);
    return handle;
}

million::resources::Handle resources::loadNamed (resources::Context* context, entt::hashed_string type, const std::string& filename, entt::hashed_string::hash_type name)
{
    EASY_BLOCK("resources::loadNamed", resources::COLOR(1));
    auto handle = resources::load(context, type, filename, name);
    if (name != 0 && handle.valid()) {
        resources::bindToName(context, handle, name);
    }
    return handle;
}

million::resources::Handle resources::find (resources::Context* context, entt::hashed_string::hash_type name)
{
    EASY_BLOCK("resources::find", resources::COLOR(1));
    auto it = context->m_named_resources.find(name);
    if (it != context->m_named_resources.end()) {
        SPDLOG_DEBUG("Resource with name {:#x} found: {:#x}", name, it->second.handle);
        return it->second;
    }
    SPDLOG_DEBUG("Resource with name {:#x} not found", name);
    return million::resources::Handle::invalid();
}

void resources::bindToName (resources::Context* context, million::resources::Handle handle, entt::hashed_string::hash_type name)
{
    EASY_BLOCK("resources::bindToName", resources::COLOR(4));
    context->m_named_resources[name] = handle;
    SPDLOG_DEBUG("Bound resource {:#x} to name {:#x}", handle.handle, name);
}

million::resources::Handle ResourceStorage::enqueue (const std::string& filename, entt::hashed_string::hash_type name)
{
    std::uint32_t id;
    if (m_loader->cached(filename, &id)) {
        return million::resources::Handle::make(m_resource_id, id);
    }
    EASY_BLOCK("resources::enqueue", resources::COLOR(3));
    million::resources::Handle handle = million::resources::Handle::make(m_resource_id, m_idx.fetch_add(1));
    m_context->m_work_queue.enqueue({m_loader, filename, handle, name});
    return handle;
}
