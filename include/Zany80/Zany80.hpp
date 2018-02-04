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
	void setRunner(liblib::Library *runner);
	
private:
	//TODO: see if instead of keeping the library pointer, caching the needed functions provides a significant speed boost
	void replaceRunner();
	liblib::Library *runner = nullptr, *plugin_manager = nullptr;
	bool attemptLoad(std::string name, liblib::Library ** library);
	
};

extern Zany80 * zany;
extern std::string folder, path;
