#pragma once

#include <Tool.hpp>

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
	bool attemptLoad(std::string name, liblib::Library ** library);
	Tool * tool;
	std::map <std::string, liblib::Library*> plugins;
	
};

extern Zany80 * zany;
