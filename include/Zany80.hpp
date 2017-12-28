#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <Tool.hpp>
#include <tools/Shell.hpp>

#define LCD_WIDTH 320
#define LCD_HEIGHT 320

class Zany80 {

public:
	Zany80();
	~Zany80();
	int run();
	void close();
	sf::RenderTexture * renderText(const char *string,sf::Color fontColor);
	sf::RenderTexture * renderText(const char *string);

private:
	sf::RenderWindow * window;
	tgui::Gui * gui;
	tgui::Canvas::Ptr canvas;
	sf::Texture font;
	Shell * shell;
	Tool * tool;
	
};
