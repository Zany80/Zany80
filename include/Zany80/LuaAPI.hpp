#pragma once

#include <lua.hpp>
#include <map>
#include <string>
#include <SFML/Window.hpp>
#include <ostream>

// Name of the table of anonymous functions
#define ANON_FUNCS "AnonymousFunctions"

void serialize(lua_State *state, std::ostream &stream);

int registerCPU(lua_State *state);
int registerExecutable(lua_State *state);
// Source and target are specified globals
void deepCopy(lua_State *source, lua_State *target, const char *table_name_source, const char *table_name_target);
// Source and target at top of stack
void deepCopy(lua_State *source, lua_State *target);
// Source and target at specified indices
void deepCopy(lua_State *source, lua_State *target, int index_src, int index_target, bool metatable = true);
/**
 * Push an anonymous Lua function from one state to another
 * Returns the address of the function within the source's registry
 */
int pushAnonymousFunction(lua_State *source, lua_State *target, int index);
/**
 * Copy a lua value from one stack to another. Supports every current 
 * type except for threads.
 * Numbers, strings, etc are duplicated. Tables make use of deepCopy.
 * Functions use pushAnonymousFunction, duplicating the function within
 * the source and creating a C closure in the target that calls it.
 */
bool copyBetweenStacks(lua_State *source, lua_State *target, int index);

void pushEventToLua(lua_State *state, sf::Event &event);

int background(lua_State *state);
int text(lua_State *state);
int millis(lua_State *state);

int btn(lua_State *state);

extern std::map<std::string, lua_CFunction> LuaAPI;
