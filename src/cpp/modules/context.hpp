#pragma once

#include <monkeys.hpp>

using CM = million::api::Module::CallbackMasks;

struct ModuleList;

namespace modules {
    struct Context {
        // API
        million::api::internal::ModuleManager* m_module_manager;
        million::api::EngineSetup* m_engine_setup;
        million::api::EngineRuntime* m_engine_runtime;

        std::shared_ptr<spdlog::logger> logger;

        ModuleList* m_modules = nullptr;

        // Module Hooks
        std::vector<million::api::Module*> m_hooks_gameSetup;
        std::vector<million::api::Module*> m_hooks_beforeFrame;
        std::vector<million::api::Module*> m_hooks_afterFrame;
        std::vector<million::api::Module*> m_hooks_physicsStep;
        std::vector<million::api::Module*> m_hooks_beforeUpdate;
        std::vector<million::api::Module*> m_hooks_loadScene;
        std::vector<million::api::Module*> m_hooks_unloadScene;
        std::vector<million::api::Module*> m_hooks_prepareRender;
        std::vector<million::api::Module*> m_hooks_beforeRender;
        std::vector<million::api::Module*> m_hooks_afterRender;
    };

    constexpr profiler::color_t COLOR(unsigned idx) {
        std::array colors{
            profiler::colors::Cyan900,
            profiler::colors::Cyan700,
            profiler::colors::Cyan500,
            profiler::colors::Cyan300,
            profiler::colors::Cyan100,
        };
        return colors[idx];
    }
}
