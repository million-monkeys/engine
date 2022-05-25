#include "scripting.hpp"
#include "context.hpp"

static int packageLoader(lua_State* state)
{
    EASY_FUNCTION(scripting::COLOR(4));
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
    EASY_FUNCTION(scripting::COLOR(2));
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

scripting::Context* scripting::init (messages::Context* msgs_ctx, events::Context* events_ctx, resources::Context* resources_ctx)
{
    EASY_BLOCK("scripting::init", scripting::COLOR(1));
    SPDLOG_DEBUG("[scripting] Init");
    scripting::Context* context = new scripting::Context;
    context->m_messages_ctx = msgs_ctx;
    context->m_events_ctx = events_ctx;
    context->m_resources_ctx = resources_ctx;

    context->m_state = luaL_newstate();
    if (!context->m_state) {
        return nullptr;
    }
    luaL_openlibs(context->m_state);
    setupPackageLoader(context->m_state);

    // Initialize scripting system
    lua_pushlightuserdata(context->m_state, reinterpret_cast<void*>(context));
    lua_setglobal(context->m_state, "MM_CONTEXT");
    luaL_dostring(context->m_state, "require('mm_init')");

    return context;
}

void scripting::setWorld (scripting::Context* context, world::Context* world_ctx)
{
    context->m_world_ctx = world_ctx;
}

void scripting::term (scripting::Context* context)
{
    EASY_BLOCK("scripting::term", scripting::COLOR(1));
    SPDLOG_DEBUG("[scripting] Term");
    lua_close(context->m_state);
}