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
	int run();
	void frame();
	void close();
	void close(std::string message);
	sf::RenderWindow *window;
	sf::Texture font;
	
private:
	std::map <std::string, liblib::Library*> plugins;
	std::vector <liblib::Library*> * runners = nullptr;
	//TODO: see if instead of keeping the library pointer, caching the needed functions provides a significant speed boost
	liblib::Library *runner = nullptr;
	
};

extern Zany80 * zany;
bool attemptLoad(std::string name, liblib::Library ** library);
