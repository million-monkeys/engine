
#include "scripting.hpp"

#include <lua.hpp>
#include <mutex>
#include <spdlog/fmt/fmt.h>

void init_scripting_api (core::Engine* engine);
void set_active_stream (entt::hashed_string stream_name);

lua_State* g_lua_state;
std::mutex g_vm_mutex;

void setLuaPath(lua_State* state, const std::string& path)
{
    lua_getglobal(state, "package");
    lua_getfield(state, -1, "path");
    std::string cur_path = lua_tostring(state, -1);
    cur_path.append(";");
    cur_path.append(path);
    lua_pop(state, 1);
    lua_pushstring(state, cur_path.c_str());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);
}

static int packageLoader(lua_State* state)
{
    std::string module_name{lua_tostring(state, -1)};
    try {
        std::string filename = module_name;
        filename.append(".lua");
        auto source = helpers::readToString(filename);
        if (luaL_loadbuffer(state, source.c_str(), source.size(), filename.c_str()) != 0) {
            spdlog::error("[script] Failed to load package '{}': {}", module_name, lua_tostring(state, -1));
            lua_pushstring(state, "");
        }
    } catch (const std::invalid_argument& e) {
        spdlog::error("[script] Could not find lua module: {}", module_name);
        lua_pushstring(state, "");
    }
	return 1;
}

bool setupPackageLoader(lua_State* state)
{
    // Remove third and fourth loader (C lib loader & combined loader)
    // Then set the custom package path
    luaL_dostring(state, R"lua(
        table.remove(package.loaders, 3)
        table.remove(package.loaders, 3)
        package.path = './?.lua;?/init.lua'
    )lua");

    // Get the package loaders list
	lua_getglobal(state, "package");
	if (lua_type(state, -1) != LUA_TTABLE) {
		spdlog::error("Lua \"package\" is not a table");
		return false;
	}
    lua_getfield(state, -1, "loaders");
    if (lua_type(state, -1) != LUA_TTABLE) {
        spdlog::error("Lua \"package.loaders\" is not a table");
        return false;
    }

    // Add the custom package loader to the end of the list
	lua_pushinteger(state, 3);
	lua_pushcfunction(state, packageLoader);
	lua_rawset(state, -3);
    lua_pop(state, 2);

    return true;
}

bool scripting::init (core::Engine* engine)
{
    init_scripting_api(engine);

    g_lua_state = luaL_newstate();
    if (!g_lua_state) {
        return false;
    }
    luaL_openlibs(g_lua_state);
    setupPackageLoader(g_lua_state);

    // Initialize scripting system
    luaL_dostring(g_lua_state, "require('mm_init')");

    // Make sure game-specific events are declared
    load(entt::monostate<"game/script-events"_hs>());

    return true;
}

void scripting::term ()
{
    lua_close(g_lua_state);
}

bool scripting::load (const std::string& filename)
{
    try {
        spdlog::debug("[script] Loading: {}", filename);
        auto source = helpers::readToString(filename);
        return evaluate(filename, source);
    } catch (const std::invalid_argument& e) {
        spdlog::error("[script] Script file not found: {}", filename);
        return false;
    }
    return true;
}

bool scripting::evaluate (const std::string& name, const std::string& source)
{
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(g_vm_mutex);

    int status = luaL_loadbuffer(g_lua_state, source.c_str(), source.size(), name.c_str());
    if (status != 0) {
        if (status == LUA_ERRSYNTAX) {
            spdlog::error("[script] Syntax error in script: {} {}", name, lua_tostring(g_lua_state, -1));
            lua_pop(g_lua_state, 1);
        } else {
            spdlog::error("[script] Could not load script: {}", name);
        }
        return false;
    }
    int ret = lua_pcall(g_lua_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(g_lua_state, -1));
        return false;
    }
    return true;
}

void scripting::processGameEvents ()
{
    EASY_BLOCK("Scripts/scene", profiler::colors::Purple100);

    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(g_vm_mutex);

    set_active_stream("game"_hs);
    lua_getglobal(g_lua_state, "handle_game_events");
    int ret = lua_pcall(g_lua_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(g_lua_state, -1));
    }
}

void scripting::processSceneEvents (million::resources::Handle handle)
{
    EASY_BLOCK("Scripts/scene", profiler::colors::Purple100);
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(g_vm_mutex);

    set_active_stream("scene"_hs);
    lua_getglobal(g_lua_state, "handle_scene_events");
    lua_pushinteger(g_lua_state, handle.id());
    int ret = lua_pcall(g_lua_state, 1, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(g_lua_state, -1));
    }
}

void scripting::processMessages ()
{
    EASY_BLOCK("Scripts/behavior", profiler::colors::Purple100);
    // Only one thread can execute Lua code at once
    std::lock_guard<std::mutex> guard(g_vm_mutex);

    set_active_stream("behavior"_hs);
    lua_getglobal(g_lua_state, "handle_messages");
    int ret = lua_pcall(g_lua_state, 0, 0, 0);
    if (ret != 0) {
        spdlog::error("[script] Runtime error {}", lua_tostring(g_lua_state, -1));
    }
}
