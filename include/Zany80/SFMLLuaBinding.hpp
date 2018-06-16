#pragma once

#include <SFML/Window.hpp>
#include <lua.hpp>

const char *getEventTypeAsString(sf::Event::EventType);
void pushEventToLua(lua_State *state, sf::Event &e);
