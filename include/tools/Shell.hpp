#pragma once

#include <Tool.hpp>
#include <TGUI/TGUI.hpp>

class Zany80;

class Shell : public Tool {
public:
	Shell(Zany80 *zenith80,tgui::Canvas::Ptr canvas);
	~Shell();
	virtual void run();
	virtual void event(sf::Event k);
private:
	Zany80 *zenith;
	tgui::Canvas::Ptr canvas;
	char * history_buffer;
	sf::RenderTexture * command;
};
