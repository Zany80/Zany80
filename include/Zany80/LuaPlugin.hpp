#pragma once

#include "Plugin.hpp"
#include <lua.hpp>

std::string getField(lua_State *L, std::string field);
static int addCapability(lua_State *L);
static int registerCPU(lua_State *L);

class LuaPlugin : public Plugin {
public:
	friend int addCapability(lua_State *L);
	LuaPlugin(lua_State *L, int index);
	LuaPlugin(lua_State *L, std::string name);
	virtual ~LuaPlugin();
	virtual std::string getName();
	virtual std::string getUpdateUrl();
	virtual std::string getDescription();
	virtual std::string getPath();
	virtual bool loadIntoRuntime();
	virtual bool provides(std::string cap);
	virtual std::string notify(Plugin *source, std::string message);
private:
	void setupLua();
	lua_State *pluginState;
	std::string name, url, desc, path;
	std::vector<std::string> capabilities;
};
