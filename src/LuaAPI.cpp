#include <Zany80/LuaAPI.hpp>
#include <Zany80/Zany80.hpp>
#include <iostream>
#include <chrono>

int registerCPU(lua_State *state) {
	if (lua_gettop(state) != 2) {
		luaL_error(state, "registerCPU called with invalid parameters");
	}
	if (!lua_istable(state, 2) || !lua_isstring(state, 1)) {
		luaL_error(state, "registerCPU called with invalid parameters");
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
	// cpus table is at position 3, which is top
	lua_newtable(L);
	lua_setfield(L, -2, lua_tostring(state, 1));
	lua_getfield(L, -1, lua_tostring(state, 1));
	deepCopy(state, L, 2, -1);
	// Retrieve a pointer to the plugin
	lua_getfield(state, LUA_REGISTRYINDEX, "plugin");
	if (lua_isuserdata(state, -1)) {
		lua_pushlightuserdata(L, lua_touserdata(state, -1));
		lua_setfield(L, -2, "plugin");
	}
	else {
		std::cerr << "Warning: Error associating CPU with plugin! If the plugin fails, it could cause an application error!\n";
	}
	lua_pop(state, 3);
	lua_pop(L, 2);
	lua_pushboolean(state, true);
	return 1;
}

void deepCopy(lua_State *source, lua_State *target, int index_src, int index_target, bool metatable) {
	// Absolutize indices
	if (index_src < 0)
		index_src = lua_gettop(source) + index_src + 1;
	if (index_target < 0)
		index_target = lua_gettop(target) + index_target + 1;
	lua_pushnil(source);
	while(lua_next(source, index_src)) {
		// Copy the key and the value to the top of the target stack, key first
		copyBetweenStacks(source, target, -2);
		copyBetweenStacks(source, target, -1);
		lua_settable(target, index_target);
		lua_pop(source, 1);
	}
	if (metatable) {
		if(lua_getmetatable(source, index_src)) {
			lua_newtable(target);
			deepCopy(source, target, -1, -1, false);
			lua_pop(source, 1);
			lua_setmetatable(target, index_target);
		}
	}
}

int pushAnonymousFunction(lua_State *source, lua_State *target, int index) {
	/* 
	 * As the function is anonymous, it needs to be stored in an 
	 * accessible manner. The method used here is to use an array table 
	 * in the Lua registry, using a static integer as the key.
	 */
	static int i = 1;
	/*
	 * Start by getting an index from the *bottom* of the stack - if
	 * index is negative, it needs to be converted so it remains valid
	 * after the AnonymousFunctions table is pushed to the top of the 
	 * stack.
	 */
	if (index < 0)
		index = lua_gettop(source) + index + 1;
	// Retrieve the AnonymousFunctions table from the registry
	lua_getfield(source, LUA_REGISTRYINDEX, ANON_FUNCS);
	if (!lua_istable(source, -1)) {
		lua_pop(source, 1);
		lua_pushstring(source, ANON_FUNCS);
		lua_newtable(source);
		lua_settable(source, LUA_REGISTRYINDEX);
		i = 1;
		lua_getfield(source, LUA_REGISTRYINDEX, ANON_FUNCS);
	}
	// Push the index to use
	lua_pushnumber(source, i);
	// Push a copy of the function
	lua_pushvalue(source, index);
	// Assign the value into the table
	lua_settable(source, -3);
	// Pop the AnonymousFunctions table
	lua_pop(source, 1);
	/* 
	 * At this point, the source has a copy of the function at index i
	 * in the AnonymousFunctions table in the registry. Now, push a C
	 * closure to the target that calls the function.
	 */
	lua_pushlightuserdata(target, source);
	lua_pushnumber(target, i);
	lua_pushcclosure(target, [](lua_State *target) -> int {
		// Retrieve the source state and the index of the anonymous func
		lua_State *source = (lua_State*)lua_touserdata(target, lua_upvalueindex(1));
		// Make sure plugins table contains source
		bool found = false;
		for (Plugin *plugin : plugins) {
			if (LuaPlugin *l = dynamic_cast<LuaPlugin*>(plugin)) {
				if (l->validateState(source)) {
					found = true;
					break;
				}
			}
		}
		if (!found) {
			lua_pushstring(target, "Plugin missing!");
			lua_error(target);
		}
		int index = lua_tonumber(target, lua_upvalueindex(2));
		// Get the top of the source stack so the number of results can
		// be calculated
		int source_top = lua_gettop(source);
		// Get the table of anonymous functions
		lua_getfield(source, LUA_REGISTRYINDEX, ANON_FUNCS);
		// Get the function
		lua_pushnumber(source, index);
		lua_gettable(source, -2);
		// Call the function
		if (lua_pcall(source, 0, LUA_MULTRET, 0) != 0) {
			// Move the error from the source to the target
			lua_pushstring(target, lua_tostring(source, -1));
			// Clean up the source and error out
			lua_pop(source, 3);
			return lua_error(target);
		}
		// Pop anonymous functions table
		lua_remove(source, source_top + 1);
		// Move results from source to target
		int results = lua_gettop(source) - source_top;
		for (int i = 0; i < results; i++) {
			int result_index = source_top + i + 1;
			copyBetweenStacks(source, target, result_index);
		}
		lua_pop(source, results);
		// Return the number of results
		return results;
	}, 2);
	i++;
}

void copyBetweenStacks(lua_State *source, lua_State *target, int index) {
	if (lua_isnil(source, index)) {
		lua_pushnil(target);
	}
	else if (lua_isnumber(source, index)) {
		lua_pushnumber(target, lua_tonumber(source, index));
	}
	else if (lua_isstring(source, index)) {
		lua_pushstring(target, lua_tostring(source, index));
	}
	else if (lua_istable(source, index)) {
		lua_newtable(target);
		deepCopy(source, target, index, -1);
	}
	else if (lua_isfunction(source, index)) {
		pushAnonymousFunction(source, target, index);
	}
}

int background(lua_State *state) {
	if (lua_gettop(state) < 3 || lua_gettop(state) > 4) {
		lua_settop(state, 0);
		luaL_error(state, "Incorrect number of parameters in call to background(r, g, b [,a])");
	}
	for (int i = 1; i <= lua_gettop(state); i++)
		if (!lua_isnumber(state, i))
			luaL_error(state, "Parameter %d is not a number in call to background!", i);
	background_color = sf::Color(
		lua_tonumber(state, 1), lua_tonumber(state, 2), lua_tonumber(state, 3));
	if (lua_gettop(state) == 4)
		background_color.a = lua_tonumber(state, 4);
	return 0;
}

int millis(lua_State *L) {
	lua_pushnumber(L, std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
	return 1;
}

std::map<std::string, lua_CFunction> LuaAPI = {
	{"validatePlugins", validatePlugins},
	{"background", background},
	{"millis", millis},
	{"registerCPU", registerCPU}
};
