#pragma once

#include <SFML/Graphics.hpp>
#include <liblib/liblib.hpp>

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

//TODO: get rid of this purposeless singleton
class Zany80 {

public:
	Zany80();
	~Zany80();
	void run();
	void frame();
	void close();
	void close(std::string message);
	sf::RenderWindow *window;
	sf::Texture font;
	void pushRunner(liblib::Library *runner);
	void popRunner();
	
private:
	liblib::Library *runner = nullptr, *plugin_manager = nullptr;
	bool attemptLoad(std::string name, liblib::Library ** library);
	
};

extern Zany80 * zany;
extern std::string folder, path, true_folder;

std::string getHomeFolder();
