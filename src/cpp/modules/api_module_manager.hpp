#pragma once

#include <monkeys.hpp>
#include <million/engine.hpp>
#include <world/world.hpp>

class ModuleManagerAPI : public million::api::internal::ModuleManager
{
public:
    ModuleManagerAPI (world::Context* world_ctx) : m_world_ctx(world_ctx) {}
    virtual ~ModuleManagerAPI () {}
    
protected:
    void* allocModule(std::size_t bytes) final
    {
        return reinterpret_cast<void*>(new std::byte[bytes]);
    }

    void deallocModule(void* ptr) final
    {
        delete [] reinterpret_cast<std::byte*>(ptr);
    }

    void prepareRegistries (million::api::definitions::PrepareFn prepareFn) final
    {
        world::prepareRegistries(m_world_ctx, prepareFn);
    }

    void installComponent (const million::api::definitions::Component& component, million::api::definitions::PrepareFn prepareFn) final
    {
        world::installComponent(m_world_ctx, component, prepareFn);
    }

private:
    world::Context* m_world_ctx;
};

