#include <tools/Shell.hpp>
#include <SFML/Window.hpp>
#include <Zany80.hpp>
#include <string.h>

Shell::Shell(Zany80 *zenith,tgui::Canvas::Ptr canvas){
	this->zenith = zenith;
	this->canvas = canvas;
	this->history_buffer = new char[4096*256];
	command = zenith->renderText("Hello, World! I now cover the full @$CII set! Isn't that amazing?");
}

Shell::~Shell(){
	delete[] history_buffer;
	delete command;
}

void Shell::run(){
	canvas->clear(sf::Color(0,0,0));
	sf::Sprite s(command->getTexture());
	s.setPosition(0,0);
	canvas->draw(s);
}

void Shell::event(sf::Event e){
	
}
