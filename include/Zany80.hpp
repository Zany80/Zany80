#pragma once

#include <SFML/Graphics.hpp>
#include <liblib/liblib.hpp>

#include <string>
#include <map>

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

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
	
private:
	//TODO: see if instead of keeping the library pointer, caching the needed functions provides a significant speed boost
	liblib::Library *runner = nullptr, *plugin_manager = nullptr;
	bool attemptLoad(std::string name, liblib::Library ** library);
	
};

extern Zany80 * zany;
