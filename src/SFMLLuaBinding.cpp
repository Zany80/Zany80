#include <Zany80/SFMLLuaBinding.hpp>
#include <map>

const char *getEventTypeAsString(sf::Event::EventType type) {
	switch (type) {
		case sf::Event::TextEntered:
			return "TextEntered";
		case sf::Event::KeyPressed:
			return "KeyPressed";
		default:
			return "Unimplemented";
	}
}

std::map<sf::Keyboard::Key, const char *> keymap = {
	// SFML added Enter as an alternative in 2.5, don't use that until 2.5
	// spreads to mainstream repositories in e.g. Debian.
	{sf::Keyboard::Return, "enter"},
	{sf::Keyboard::BackSpace, "backspace"},
	{sf::Keyboard::Up, "up"},
	{sf::Keyboard::Down, "down"}
};

void pushEventToLua(lua_State *state, sf::Event &e) {
	lua_newtable(state);
	lua_pushstring(state, getEventTypeAsString(e.type));
	lua_setfield(state, -2, "type");
	switch (e.type) {
		case sf::Event::TextEntered:
			lua_pushnumber(state, e.text.unicode);
			lua_setfield(state, -2, "character");
			break;
		case sf::Event::KeyPressed:
			if (keymap.find(e.key.code) == keymap.end())
				break;
			lua_pushstring(state, keymap[e.key.code]);
			lua_setfield(state, -2, "code");
			break;
	}
}
