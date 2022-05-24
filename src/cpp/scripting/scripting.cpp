
#include "scripting.hpp"
#include "context.hpp"

#include <spdlog/fmt/fmt.h>
#include <lua.hpp>

void set_active_stream (scripting::Context* context, entt::hashed_string stream_name);

bool scripting::load (scripting::Context* context, const std::string& filename)
{
    EASY_BLOCK("Scripts/load", scripting::COLOR(1));
    SPDLOG_TRACE("[script] Loading script");
    try {
        spdlog::debug("[script] Loading: {}", filename);
        auto source = helpers::readToString(filename);
        return evaluate(context, filename, source);
    } catch (const std::invalid_argument& e) {
        spdlog::error("[script] Script file not found: {}", filename);
        return false;
    }
    return true;
}

bool scripting::evaluate (scripting::Context* context, const std::string& name, const std::string& source)
{
    EASY_BLOCK("Scripts/evaluate", scripting::COLOR(2));
    SPDLOG_TRACE("[script] Evaluating code");
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(context->m_vm_mutex);

    int status = luaL_loadbuffer(context->m_state, source.c_str(), source.size(), name.c_str());
    if (status != 0) {
        if (status == LUA_ERRSYNTAX) {
            spdlog::error("[script] Syntax error in script: {} {}", name, lua_tostring(context->m_state, -1));
            lua_pop(context->m_state, 1);
        } else {
            spdlog::error("[script] Could not load script: {}", name);
        }
        return false;
    }
    int ret = lua_pcall(context->m_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(context->m_state, -1));
        return false;
    }
    return true;
}

void scripting::detail::call (scripting::Context* context, const std::string& function, const scripting::detail::VariantVector& args)
{
    EASY_BLOCK("Scripts/call", scripting::COLOR(1));
    SPDLOG_TRACE("[script] Calling function {}", function);

    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(context->m_vm_mutex);

    lua_getglobal(context->m_state, function.c_str());
    for (const auto& arg : args) {
        std::visit(
            helpers::visitor{
                [context](const std::string& str) { lua_pushstring(context->m_state, str.c_str()); },
                [context](const char* str) { lua_pushstring(context->m_state, str); },
                [context](int num) { lua_pushinteger(context->m_state, num); },
                [context](long num) { lua_pushinteger(context->m_state, num); },
                [context](float num) { lua_pushnumber(context->m_state, num); },
                [context](double num) { lua_pushnumber(context->m_state, num); },
                [context](bool boolean) { lua_pushboolean(context->m_state, boolean); },
                [context](void* ptr) { lua_pushlightuserdata(context->m_state, ptr); }
            }, arg);
    }
    int ret = lua_pcall(context->m_state, args.size(), 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(context->m_state, -1));
        throw std::runtime_error("Script encountered unrecoverable error");
    }
}


void scripting::processGameEvents (scripting::Context* context)
{
    EASY_BLOCK("Scripts/scene", scripting::COLOR(1));
    SPDLOG_TRACE("[script] Processing game event scripts");

    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(context->m_vm_mutex);

    set_active_stream(context, "game"_hs);
    lua_getglobal(context->m_state, "handle_game_events");
    int ret = lua_pcall(context->m_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(context->m_state, -1));
        throw std::runtime_error("Script encountered unrecoverable error");
    }
}

void scripting::processSceneEvents (scripting::Context* context, million::resources::Handle handle)
{
    EASY_BLOCK("Scripts/scene", scripting::COLOR(1));
    SPDLOG_TRACE("[script] Processing scene event scripts");
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(context->m_vm_mutex);

    set_active_stream(context, "scene"_hs);
    lua_getglobal(context->m_state, "handle_scene_events");
    lua_pushinteger(context->m_state, handle.id());
    int ret = lua_pcall(context->m_state, 1, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(context->m_state, -1));
        throw std::runtime_error("Script encountered unrecoverable error");
    }
}

void scripting::processMessages (scripting::Context* context)
{
    EASY_BLOCK("Scripts/behavior", scripting::COLOR(1));
    SPDLOG_TRACE("[script] Processing scripted behaviors");
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(context->m_vm_mutex);

    set_active_stream(context, "behavior"_hs);
    lua_getglobal(context->m_state, "handle_messages");
    int ret = lua_pcall(context->m_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(context->m_state, -1));
        throw std::runtime_error("Script encountered unrecoverable error");
    }
}

void scripting::registerComponent (scripting::Context* context, entt::hashed_string::hash_type name, entt::id_type id)
{
    context->m_component_types[name] = id;
}
