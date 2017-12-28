#pragma once

#include <Tool.hpp>
#include <TGUI/TGUI.hpp>

class Zenith80;

class Shell : public Tool {
public:
	Shell(Zenith80 *zenith80,tgui::Canvas::Ptr canvas);
	~Shell();
	virtual void run();
	virtual void event(sf::Event k);
private:
	Zenith80 *zenith;
	tgui::Canvas::Ptr canvas;
	char * history_buffer;
	sf::RenderTexture * command;
};
