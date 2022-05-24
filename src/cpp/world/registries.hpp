#pragma once

#include "registry_pair.hpp"

struct Registries {
public:
    RegistryPair& foreground() { return m_registries[m_foreground_registry]; }
    const RegistryPair& foreground() const { return m_registries[m_foreground_registry]; }
    RegistryPair& background() { return m_registries[1 - m_foreground_registry]; }
    const RegistryPair& background() const { return m_registries[1 - m_foreground_registry]; }
    void swap () { m_foreground_registry = 1 - m_foreground_registry; }            

    // Copy all entities from one registry to another
    void copyRegistry (const entt::registry& from, entt::registry& to);
    // Copy "global" entities from background to foreground
    void copyGlobals ();
private:
    RegistryPair m_registries[2];
    std::uint32_t m_foreground_registry = 0;
};