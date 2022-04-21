
#include "core.hpp"
#include <lua.hpp>
#include <spdlog/fmt/fmt.h>

void set_engine (core::Engine* engine);

scripting::Engine::Engine (core::Engine* engine)
{
    set_engine(engine);
}

scripting::Engine::~Engine ()
{

}

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

bool scripting::Engine::init ()
{
    m_lua_state = luaL_newstate();
    if (!m_lua_state) {
        return false;
    }
    luaL_openlibs(m_lua_state);
    setupPackageLoader(m_lua_state);
    luaL_dostring(m_lua_state, "require('core_components')");

    return true;
}

void scripting::Engine::term ()
{
    lua_close(m_lua_state);
}

bool scripting::Engine::load (const std::string& filename)
{
    try {
        spdlog::debug("[script] Loading: {}", filename);
        auto source = helpers::readToString(filename);
        int status = luaL_loadbuffer(m_lua_state, source.c_str(), source.size(), filename.c_str());
        if (status != 0) {
            if (status == LUA_ERRSYNTAX) {
                spdlog::error("[script] Syntax error in script: {}", filename);
            } else {
                spdlog::error("[script] Could not load script: {}", filename);
            }
            return false;
        }
        int ret = lua_pcall(m_lua_state, 0, 0, 0);
        if (ret != 0) {
            spdlog::error("[script] Runtime error {}", lua_tostring(m_lua_state, -1));
            return false;
        }
    } catch (const std::invalid_argument& e) {
        spdlog::error("[script] Script file not found: {}", filename);
        return false;
    }
    return true;
}
