#pragma once

#include <Tool.hpp>
#include <TGUI/TGUI.hpp>
#include <Text.hpp>

class Zany80;

class Shell : public Tool {
public:
	Shell(tgui::Canvas::Ptr canvas);
	~Shell();
	virtual void run();
	virtual void event(sf::Event k);
private:
	tgui::Canvas::Ptr canvas;
	Text command;
};
