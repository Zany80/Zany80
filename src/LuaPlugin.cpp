#include <Zany80/LuaPlugin.hpp>
#include <Zany80/Zany80.hpp>
#include <chrono>
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
	close_state(&pluginState);
}

static int millis(lua_State *L) {
	lua_pushnumber(L, std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
	return 1;
}

void LuaPlugin::setupLua() {
	pluginState = luaL_newstate();
	lua_pushcfunction(pluginState, luaopen_base);
	lua_pushstring(pluginState, "");
	lua_call(pluginState, 1, 0);
	lua_pushlightuserdata(pluginState, this);
	lua_pushcclosure(pluginState, addCapability, 1);
	lua_setglobal(pluginState, "registerCapability");
	lua_pushcfunction(pluginState, registerCPU);
	lua_setglobal(pluginState, "registerCPU");
	lua_pushcfunction(pluginState, luaopen_math);
	lua_pushstring(pluginState, LUA_MATHLIBNAME);
	lua_call(pluginState, 1, 0);
	lua_pushcfunction(pluginState, millis);
	lua_setglobal(pluginState, "millis");
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

// TODO: is this really needed? It doesn't seem to be...
static int addCapability(lua_State *L) {
	LuaPlugin *plugin = (LuaPlugin*)lua_touserdata(L, lua_upvalueindex(1));
	if (!lua_gettop(L)) {
		lua_pushstring(L, "At least one argument required!");
		lua_error(L);
	}
	for (int i = 1; i <= lua_gettop(L); i++) {
		if (!lua_isstring(L, i)) {
			lua_pushstring(L,"Only strings allowed!");
			lua_error(L);
		}
		plugin->capabilities.push_back(lua_tostring(L, i));
		//std::cout << "Capability "<<lua_tostring(L, i) << " provided by plugin "<<plugin->getName()<<'\n';
	}
	return 0;
}

static int registerCPU(lua_State *state) {
	if (lua_gettop(state) != 1) {
		lua_pushboolean(state, false);
		lua_error(state);
	}
	if (!lua_istable(state, 1)) {
		lua_pushstring(state, "CPU argument must be a table!");
		lua_error(state);
	}
	lua_getglobal(L, "cpus");
	if (!lua_istable(L, -1)) {
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
			lua_setglobal(L, "cpus");
			lua_getglobal(L, "cpus");
		}
		else {
			close("cpus must be a table!");
		}
	}
	// cpu arg is at 1 in state, target table is at -1 in L
	
	lua_pushboolean(state, true);
	return 1;
}

static int registerRunner(lua_State *state) {
	
}
