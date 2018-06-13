#include <Zany80/LuaAPI.hpp>
#include <Zany80/LuaPlugin.hpp>
#include <Zany80/Zany80.hpp>
#include <exception>
#include <iostream>

LuaPlugin::LuaPlugin(lua_State *L, int index) {
	lua_getglobal(L, "plugins");
	// Don't bother verifying that it's a table; it's already been done,
	// and doing it for every plugin would be overkill.
	lua_pushnumber(L, index);
	lua_gettable(L, -2);
	// That doesn't mean that we shouldn't verify individual plugins!
	if (!lua_istable(L, -1)) {
		lua_pop(L, 2);
		throw std::exception();
	}
	try {
		name = getField(L, "name");
		desc = getField(L, "description");
		path = getField(L, "path");
		try {
			url = getField(L, "update_url");
		}
		catch (std::exception &e) {
			url = "";
		}
	}
	catch (std::exception &e) {
		lua_pop(L, 2);
		throw e;
	}
	lua_pop(L, 2);
	setupLua();
}

LuaPlugin::~LuaPlugin() {
	lua_getglobal(pluginState, "cleanup");
	if (lua_isfunction(pluginState, -1)) {
		if (lua_pcall(pluginState, 0, 0, 0)) {
			// Try using the plugin's log function
			std::string error = lua_tostring(pluginState, -1);
			if (error.find(".lua:") != std::string::npos) {
				std::string file_name = error.substr(0, error.find(".lua:"));
				// Get second to last forward slash
				int last_slash = file_name.find_last_of("/");
				if (last_slash != std::string::npos) {
					int target_slash = file_name.substr(0, last_slash - 1)
						.find_last_of("/");
					if (target_slash != std::string::npos) {
						error = "\u001b[31mError: " + error.substr(target_slash + 1)
							+ "\u001b[0m";
					}
				}
			}
			lua_getglobal(pluginState, "log");
			if (lua_isfunction(pluginState, -1)) {
				lua_pushstring(pluginState, error.c_str());
				if (lua_pcall(pluginState, 1, 0, 0)) {
					// Failed to use plugin's logger!
					std::cerr << "Error: "<<error<<'\n';
				}
			}
			else {
				// Plugin has no logger!
				std::cerr << "Error: "<<error<<'\n';
			}
		}
	}
	close_state(&pluginState);
}

void LuaPlugin::setupLua() {
	pluginState = luaL_newstate();
	lua_pushcfunction(pluginState, luaopen_base);
	lua_pushstring(pluginState, "");
	lua_call(pluginState, 1, 0);
	lua_pushcfunction(pluginState, luaopen_string);
	lua_pushstring(pluginState, LUA_STRLIBNAME);
	lua_call(pluginState, 1, 0);
	lua_pushcfunction(pluginState, luaopen_math);
	lua_pushstring(pluginState, LUA_MATHLIBNAME);
	lua_call(pluginState, 1, 0);
	for (auto pair : LuaAPI) {
		lua_pushcfunction(pluginState, pair.second);
		lua_setglobal(pluginState, pair.first.c_str());
	}
	lua_pushlightuserdata(pluginState, L);
	lua_pushcclosure(pluginState, [](lua_State *state) -> int {
		lua_State *L = (lua_State*) lua_touserdata(state, lua_upvalueindex(1));
		if (lua_gettop(state) == 1) {
			if (lua_isnumber(state, 1)) {
				lua_pushnumber(L, lua_tonumber(state, 1));
				lua_setglobal(L, "depth");
				return 0;
			}
			else {
				lua_pushstring(state, "\u001b[31mDepth must be a number!\u001b[00m");
				lua_error(state);
			}
		}
		else {
			lua_getglobal(L, "depth");
			lua_pushnumber(state, lua_tonumber(L, -1));
			lua_pop(L, 1);
			return 1;
		}
	}, 1);
	lua_setglobal(pluginState, "depth");
	lua_pushlightuserdata(pluginState, this);
	lua_setfield(pluginState, LUA_REGISTRYINDEX, "plugin");
}

std::string LuaPlugin::getName() {
	return this->name;
}

std::string LuaPlugin::getDescription() {
	return this->desc;
}

std::string LuaPlugin::getUpdateUrl() {
	return this->url;
}

std::string LuaPlugin::getPath() {
	return this->path;
}

bool LuaPlugin::loadIntoRuntime() {
	if (luaL_dofile(pluginState, (folder + "/init_plugins.lua").c_str()))
		close(lua_tostring(pluginState, -1));
	if (luaL_dofile(pluginState, (this->path + "/main.lua").c_str())) {
		desc = lua_tostring(pluginState, -1);
		return false;
	}
	return true;
}

std::string getField(lua_State *L, std::string field) {
	lua_getfield(L, -1, field.c_str());
	if (!lua_isstring(L, -1)) {
		lua_remove(L, -1);
		throw std::exception();
	}
	std::string value = lua_tostring(L, -1);
	lua_remove(L, -1);
	if (field == "path") {
		size_t var;
		while ((var = value.find("$ZANY")) != std::string::npos) {
			std::string first_part = value.substr(0, var);
			std::string end_part = value.substr(var + 5, value.size() - var - 5);
			value = first_part + folder.substr(0, folder.size() - 1) + end_part;
		}
	}
	return value;
}

bool LuaPlugin::provides(std::string cap) {
	for (std::string c : capabilities) {
		if (c == cap)
			return true;
	}
	return false;
}

std::string LuaPlugin::notify(Plugin *source, std::string message) {
	lua_getglobal(pluginState, "messageHandler");
	if (!lua_isfunction(pluginState, -1)) {
		lua_pop(pluginState, 1);
		return NOTIF_UNACCEPTED;
	}
	lua_pushstring(pluginState, message.c_str());
	if (lua_pcall(pluginState, 1, 1, 0)) {
		std::cerr << 	"[LuaPlugin.notify] Error notifying plugin!\n"
						"\t[Lua] "<<lua_tostring(pluginState, -1) << '\n';
		lua_pop(pluginState, 1);
		return NOTIF_ERROR;
	}
	const char *msg = lua_tostring(pluginState, -1);
	lua_pop(pluginState, 1);
	if (!msg)
		return "";
	return msg;
}
