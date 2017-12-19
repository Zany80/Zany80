#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <Tool.hpp>
#include <tools/Shell.hpp>

class Zenith80 {

public:
	Zenith80();
	~Zenith80();
	int run();
	void close();

private:
	sf::RenderWindow * window;
	tgui::Gui * gui;
	tgui::Canvas::Ptr canvas;
	Shell * shell;
	Tool * tool;
	
};
