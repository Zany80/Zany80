#pragma once

#include "Plugin.hpp"
#include <lua.hpp>

std::string getField(lua_State *L, std::string field);

class LuaPlugin : public Plugin {
public:
	LuaPlugin(lua_State *L, int index);
	LuaPlugin(lua_State *L, std::string name);
	virtual ~LuaPlugin(){}
	virtual std::string getName();
	virtual std::string getUpdateUrl();
	virtual std::string getDescription();
	virtual std::string getPath();
	virtual void loadIntoRuntime();
private:
	std::string name, url, desc, path;
};
