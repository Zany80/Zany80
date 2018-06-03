#pragma once

#include <SFML/Graphics.hpp>
#include <liblib/liblib.hpp>

#include <vector>
#include <string>
#include <map>

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

#define GLYPH_WIDTH 6
#define GLYPH_HEIGHT 12
#define FONT_WIDTH 96
#define FONT_HEIGHT 72

#define GLYPHS_PER_ROW (FONT_WIDTH / GLYPH_WIDTH)
#define GLYPHS_PER_COLUMN (FONT_HEIGHT / GLYPH_HEIGHT)

// Generic message type
typedef struct {
	// intended to be used as a way of determining *relative* priority, but can be used as a "hack" to ensure a message arrives if sender and receiver agree.
	// normal priority is 0
	int priority;
	const char *data;
	int length;
	const char *source;
	// What caused this message to be sent? Did a plugin request unavailable info?
	// Also used in some cases to pass a generic pointer - when data is "init",
	// context contains the address of the plugin manager
	const char *context;
} PluginMessage;

typedef void (*message_t)(PluginMessage,const char *);
typedef void (*textMessage_t)(const char *,const char *);
typedef void (*broadcast_t)(PluginMessage);

extern std::string folder, path;
extern sf::RenderWindow window;
extern sf::Texture font;
extern sf::Color background;

typedef struct {
	std::string name;
	std::string description;
	std::string path;
} PluginDescriptor;

void close(std::string message);
std::string absolutize(std::string relative_path);
void initializePlugins();
std::vector<PluginDescriptor> gatherPlugins();
std::string hex(long num, int length = 8);
