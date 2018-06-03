#include <Zany80/LuaPlugin.hpp>
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
		for (int i = 0; i < 2; i++)
			lua_remove(L, -1);
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
		for (int i = 0; i < 2; i++)
			lua_remove(L, -1);
		throw e;
	}
	for (int i = 0; i < 2; i++)
		lua_remove(L, -1);
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

extern lua_State *L;

void LuaPlugin::loadIntoRuntime() {
	// Clear the stack
	lua_settop(L, 0);
	if (luaL_dofile(L, (this->path + "/main.lua").c_str())) {
		// Error!
		std::cerr << "Error loading plugin " << getName() << "!\n";
		std::cerr << lua_tostring(L, -1) << '\n';
	}
	lua_settop(L, 0);
}

// Faster than including the entire Zany80.hpp header
extern std::string folder;
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
