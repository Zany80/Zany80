#pragma once

#include <SFML/Graphics.hpp>
#include <liblib/liblib.hpp>
#include <Zany80/LuaPlugin.hpp>

#include <vector>
#include <string>
#include <map>

#include <lua.hpp>

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

#define GLYPH_WIDTH 6
#define GLYPH_HEIGHT 12
#define FONT_WIDTH 96
#define FONT_HEIGHT 72

#define GLYPHS_PER_ROW (FONT_WIDTH / GLYPH_WIDTH)
#define GLYPHS_PER_COLUMN (FONT_HEIGHT / GLYPH_HEIGHT)

extern std::string folder, path;
extern sf::RenderWindow window;
extern sf::Texture font;
extern sf::Color background_color;
extern lua_State *L;
extern std::vector<Plugin*> plugins;

void close(std::string message);
std::string absolutize(std::string relative_path);
std::string hex(long num, int length = 8);

class LuaPlugin;
std::vector<LuaPlugin*> gatherLuaPlugins();
std::vector<Plugin*> gatherPlugins();
void initializeGlobalLua();
void initializePlugins();
void cleanupPlugins();
int validatePlugins(lua_State *L);

void close_state(lua_State **state);
#define autocl __attribute__((cleanup(close_state)))

std::string getConfigDirectory();
lua_State *get_configuration();
