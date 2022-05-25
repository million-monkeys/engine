
#include "scripting.hpp"
#include "context.hpp"
#include "core/engine.hpp"

#include "resources/resources.hpp"

#include <stdexcept>

extern "C" std::uint32_t get_entity_set (scripting::Context* context, std::uint32_t, const std::uint32_t**)
{
    return 0;
}

extern "C" void output_log (scripting::Context* context, std::uint32_t level, const char* message)
{
    EASY_FUNCTION(scripting::COLOR(3));
    switch (level) {
    case 0:
        spdlog::critical("[script] {}", message);
        break;
    case 1:
        spdlog::error("[script] {}", message);
        break;
    case 2:
    {
        spdlog::warn("[script] {}", message);
        const bool& terminate_on_error = entt::monostate<"engine/terminate-on-error"_hs>();
        if (terminate_on_error) {
            throw std::runtime_error("Script Error");
        }
        break;
    }
    case 3:
        spdlog::info("[script] {}", message);
        break;
    case 4:
        SPDLOG_DEBUG("[script] {}", message);
        break;
    default:
        spdlog::warn("[script] invalid log level: {}", level);
        break;
    }
}

extern "C" std::uint32_t get_ref (const char* name)
{
    return entt::hashed_string::value(name);
}

extern "C" std::uint32_t load_resource (scripting::Context* context, const char* type, const char* filename, const char* name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    auto handle = resources::loadNamed(context->m_resources_ctx, entt::hashed_string{type}, std::string{filename}, name != nullptr ? entt::hashed_string::value(name) : 0);
    return handle.handle;
}

extern "C" std::uint32_t find_resource (scripting::Context* context, const char* name)
{
    EASY_FUNCTION(scripting::COLOR(3));
    auto handle = resources::find(context->m_resources_ctx, entt::hashed_string::value(name));
    return handle.handle;
}
