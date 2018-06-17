#include <Zany80/Drawing.hpp>
#include <Zany80/LuaAPI.hpp>
#include <Zany80/Zany80.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

int registerTable(lua_State *state, const char *table) {
	lua_getglobal(L, table);
	if (!lua_istable(L, -1)) {
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
			lua_setglobal(L, table);
			lua_getglobal(L, table);
		}
		else {
			close(table + std::string(" must be a table!"));
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
		std::cerr << "Warning: Error associating object with plugin! If the plugin fails, it could cause an application error!\n";
	}
	lua_pop(state, 3);
	lua_pop(L, 2);
	lua_pushboolean(state, true);
	return 1;
}

int registerCPU(lua_State *state) {
	if (lua_gettop(state) != 2) {
		luaL_error(state, "registerCPU called with invalid parameters");
	}
	if (!lua_istable(state, 2) || !lua_isstring(state, 1)) {
		luaL_error(state, "registerCPU called with invalid parameters");
	}
	registerTable(state, "cpus");
}

int registerExecutable(lua_State *state) {
	if (lua_gettop(state) != 2) {
		luaL_error(state, "registerExecutable called with invalid parameters");
	}
	if (!lua_istable(state, 2) || !lua_isstring(state, 1)) {
		luaL_error(state, "registerExecutable called with invalid parameters");
	}
	registerTable(state, "executables");
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
		if (!copyBetweenStacks(source, target, -2))
			continue;
		if (!copyBetweenStacks(source, target, -1)) {
			lua_pop(target, 1);
			continue;
		}
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
		int index = lua_tonumber(target, lua_upvalueindex(2));
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
			luaL_error(target, "Plugin missing!");
		}
		// Make sure stack is clean
		lua_settop(source, 0);
		// Get the table of anonymous functions
		lua_getfield(source, LUA_REGISTRYINDEX, ANON_FUNCS);
		// Get the function
		lua_rawgeti(source, 1, index);
		lua_remove(source, 1);
		int args = lua_gettop(target);
		// Copy over arguments from target to source
		for (int i = 1; i <= args; i++) {
			copyBetweenStacks(target, source, i);
		}
		lua_settop(target, 0);
		// Call the function
		if (lua_pcall(source, args, LUA_MULTRET, 0) != 0) {
			// Move the error from the source to the target
			lua_pushstring(target, lua_tostring(source, -1));
			// Clean up the source and error out
			lua_settop(source, 0);
			return lua_error(target);
		}
		// Move results from source to target
		int results = lua_gettop(source);
		for (int i = 1; i <= results; i++) {
			copyBetweenStacks(source, target, i);
		}
		lua_settop(source, 0);
		// Return the number of results
		return results;
	}, 2);
	i++;
}

bool copyBetweenStacks(lua_State *source, lua_State *target, int index) {
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
	else if (lua_isboolean(source, index)) {
		lua_pushboolean(target, lua_toboolean(source, index));
	}
	else
		return false;
	return true;
}

int background(lua_State *state) {
	if (lua_gettop(state) < 3 || lua_gettop(state) > 4) {
		//lua_settop(state, 0);
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

int millis(lua_State *state) {
	lua_pushnumber(state, std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
	return 1;
}

int text(lua_State *state) {
	if (lua_gettop(state) < 3 
			|| (lua_gettop(state) > 3 && (lua_gettop(state) != 6 && lua_gettop(state) != 7))
			|| !lua_isstring(state, 1)) {
		luaL_error(state, "Usage: text(string, x, y[, r, g, b[, a]])");
	}
	for (int i = 2; i <= lua_gettop(state); i++) {
		if (!lua_isnumber(state, i)) {
			luaL_error(state, "Usage: text(string, x, y[, r, g, b[, a]])");
		}
	}
	const char *string = lua_tostring(state, 1);
	double x = lua_tonumber(state, 2), y = lua_tonumber(state, 3);
	sf::Color color = sf::Color::White;
	if (lua_gettop(state) > 3) {
		color.r = lua_tonumber(state, 4);
		color.g = lua_tonumber(state, 5);
		color.b = lua_tonumber(state, 6);
		if (lua_gettop(state) == 7) {
			color.a = lua_tonumber(state, 7);
		}
	}
	text(string, x, y, color);
	return 0;
}

int btn(lua_State *state) {
	if (!lua_gettop(state) == 1 || !lua_isnumber(state, 1))
		luaL_error(state, "Usage: btn(button_to_check)");
	switch (lua_tointeger(state, 1)) {
		case 1:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::Up));
			break;
		case 2:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::Down));
			break;
		case 3:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::Left));
			break;
		case 4:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::Right));
			break;
		case 5:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::Z));
			break;
		case 6:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::X));
			break;
		case 7:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::A));
			break;
		case 8:
			lua_pushboolean(state, sf::Keyboard::isKeyPressed(sf::Keyboard::S));
			break;
		default:
			lua_pushboolean(state, false);
			break;
	}
	return 1;
}

void serializeTable(lua_State *state, std::ostream &stream, int index, int depth, const char *name = "Unnamed") {
	stream << "{\n";
	lua_pushnil(state);
	while (lua_next(state, index) != 0) {
		for (int i = 0; i < depth; i++)
			stream << '\t';
		if (lua_isnumber(state, -2))
			stream << '[' << lua_tonumber(state, -2) << "] = ";
		else
			stream << "[\"" << lua_tostring(state, -2) << "\"] = ";
		if (lua_isnumber(state, -1))
			stream << lua_tonumber(state, -1);
		else if (lua_isstring(state, -1))
			stream << '"'<<lua_tostring(state, -1)<<'"';
		else if (lua_isboolean(state, -1))
			stream << (lua_toboolean(state, -1) ? "true" : "false");
		else if (lua_istable(state, -1))
			serializeTable(state, stream, lua_gettop(state), depth + 1);
		stream << ", \n";
		lua_pop(state, 1);
	}
	for (int i = 1; i < depth; i++)
		stream << '\t';
	stream << "}";
}

void serialize(lua_State *state, std::ostream &stream) {
	lua_checkstack(state, 100);
	lua_pushnil(state);
	while(lua_next(state, LUA_GLOBALSINDEX) != 0) {
		stream << lua_tostring(state, -2) << " = ";
		if (lua_isnumber(state, -1))
			stream << lua_tonumber(state, -1) << '\n';
		else if (lua_isstring(state, -1))
			stream << '"'<<lua_tostring(state, -1) << "\"\n";
		else if (lua_isboolean(state, -1))
			stream << (lua_toboolean(state, -1) ? "true" : "false") << '\n';
		else if (lua_istable(state, -1)) {
			serializeTable(state, stream, lua_gettop(state), 1, lua_tostring(state, -2));
			stream << '\n';
		}
		lua_pop(state, 1);
	}
}

void setFilePerm(lua_State *configuration, int key, const char *permission, bool value, const char *file) {
	lua_getglobal(configuration, "plugins");
	lua_rawgeti(configuration, -1, key);
	lua_getfield(configuration, -1, "permissions");
	if (!lua_istable(configuration, -1)) {
		lua_pop(configuration, 1);
		lua_newtable(configuration);
		lua_setfield(configuration, -2, "permissions");
		lua_getfield(configuration, -1, "permissions");
	}
	lua_getfield(configuration, -1, permission);
	if (!lua_istable(configuration, -1)) {
		lua_pop(configuration, 1);
		lua_newtable(configuration);
		lua_setfield(configuration, -2, permission);
		lua_getfield(configuration, -1, permission);
	}
	lua_pushboolean(configuration, value);
	lua_setfield(configuration, -2, file);
	lua_pop(configuration, 4);
}

int readFile(lua_State *state) {
	if (!lua_isstring(state, 1))
		luaL_error(state, "readFile argument must be a string!");
	autocl lua_State *configuration = get_configuration();
	// First, figure out which Plugin the state belongs to
	bool found = false;
	LuaPlugin *plugin;
	for (Plugin *p : plugins) {
		if (LuaPlugin *l = dynamic_cast<LuaPlugin*>(p)) {
			if (l->validateState(state)) {
				found = true;
				plugin = l;
				break;
			}
		}
	}
	if (!found)
		luaL_error(state, "Unable to identify plugin!");
	lua_getglobal(configuration, "plugins");
	int index = lua_gettop(configuration);
	found = false;
	lua_pushnil(configuration);
	while(lua_next(configuration, index)) {
		if (lua_istable(configuration, -1)) {
			lua_getfield(configuration, -1, "name");
			if (lua_isstring(configuration, -1)) {
				if (plugin->getName() == lua_tostring(configuration, -1)) {
					lua_pop(configuration, 2);
					found = true;
					break;
				}
			}
			lua_pop(configuration, 1);
		}
		lua_pop(configuration, 1);
	}
	if (!found)
		luaL_error(state, "Key not found");
	if (!lua_isstring(configuration, -1))
		luaL_error(state, "Configuration is invalid?!");
	if (!lua_isnumber(configuration, -1))
		luaL_error(state, "Plugin key isn't a number!");
	int key = lua_tonumber(configuration, -1);
	std::string file = lua_tostring(state, 1);
	lua_settop(state, 0);
	// Check if permission is granted within configuration
	int permission_granted = -1;
	lua_rawgeti(configuration, index, key);
	lua_getfield(configuration, -1, "permissions");
	if (lua_istable(configuration, -1)) {
		lua_getfield(configuration, -1, "read");
		if (lua_istable(configuration, -1)) {
			lua_getfield(configuration, -1, file.c_str());
			if (lua_isboolean(configuration, -1)) {
				permission_granted = lua_toboolean(configuration, -1);
			}
			lua_pop(configuration, 1);
		}
		lua_pop(configuration, 1);
	}
	lua_pop(configuration, 1);
	switch(permission_granted) {
		case -1:
			// No permanent decision made. Need to request approval.
			while (true) {
				sf::Event event;
				while(window.pollEvent(event)) {
					if (event.type == sf::Event::Closed) {
						window.close();
						exit(0);
					}
					else if (event.type == sf::Event::KeyPressed) {
						switch(event.key.code) {
							case sf::Keyboard::A:
								// Allow permanently. Store the configuration, then do the same as Z
								setFilePerm(configuration, key, "read", true, file.c_str());
								{
									std::ofstream config_file(getConfigDirectory() + "/config.lua", std::ios::trunc);
									serialize(configuration, config_file);
									config_file.close();
								}
							case sf::Keyboard::Z:
								// Allow just this once.
								// clear event queue
								sf::Event e;
								while(window.pollEvent(e));
								{
									std::ifstream file_stream(file, std::ios::ate);
									if (file_stream.is_open()) {
										int size = file_stream.tellg();
										file_stream.seekg(0, std::ios::beg);
										char *buffer = new char[size];
										file_stream.read(buffer, size);
										file_stream.close();
										lua_pushstring(state, buffer);
										delete[] buffer;
										return 1;
									}
									else {
										return luaL_error(state, "Error opening file!");
									}
								}
							case sf::Keyboard::S:
								setFilePerm(configuration, key, "read", false, file.c_str());
								{
									std::ofstream config_file(getConfigDirectory() + "/config.lua", std::ios::trunc);
									serialize(configuration, config_file);
									config_file.close();
								}
							case sf::Keyboard::X:
								{
									sf::Event e;
									while(window.pollEvent(e));
								}
								return luaL_error(state, "Permission denied.");
						}
					}
				}
				window.clear(sf::Color::Black);
				text("Permission requested: ", 0, 0, sf::Color::White);
				text("Plugin " + plugin->getName() + " requests read access to \"" + file + '"', 0, GLYPH_HEIGHT * 2.4, sf::Color::White);
				text("Press Z to allow.", 0, GLYPH_HEIGHT * 4, sf::Color::White);
				text("Press X to deny.", 0, GLYPH_HEIGHT * 5, sf::Color::White);
				text("Press A to permanently allow.", 0, GLYPH_HEIGHT * 6, sf::Color::White);
				text("This will allow this plugin to read this file without asking in the future.", GLYPH_WIDTH * 4, GLYPH_HEIGHT * 7, sf::Color::White);
				text("Press S to permanently deny.", 0, GLYPH_HEIGHT * 8, sf::Color::White);
				text("This will automatically deny all future requests to access this file.", GLYPH_WIDTH * 4, GLYPH_HEIGHT * 9, sf::Color::White);
				window.display();
			}
			break;
		case 0:
			// Permission explicitly denied.
			return luaL_error(state, "Permission denied.");
		case 1:
			// Permission granted!
			std::ifstream file_stream(file, std::ios::ate);
			if (file_stream.is_open()) {
				int size = file_stream.tellg();
				file_stream.seekg(0, std::ios::beg);
				char *buffer = new char[size];
				file_stream.read(buffer, size);
				file_stream.close();
				lua_pushstring(state, buffer);
				delete[] buffer;
				return 1;
			}
			else {
				return luaL_error(state, "Error opening file!");
			}
			break;
	}
	return 0;
}

std::map<std::string, lua_CFunction> LuaAPI = {
	{"validatePlugins", validatePlugins},
	{"background", background},
	{"text", text},
	{"millis", millis},
	{"registerCPU", registerCPU},
	{"registerExecutable", registerExecutable},
	{"btn", btn},
	{"readFile", readFile}
};
