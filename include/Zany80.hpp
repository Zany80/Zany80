#pragma once

#include <Tool.hpp>

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

class Zany80 {

public:
	Zany80();
	~Zany80();
	int run();
	void frame();
	void close();
	sf::RenderWindow *window;
	sf::Texture font;
	
private:
	Tool * tool;
	
};

extern Zany80 * zany;
