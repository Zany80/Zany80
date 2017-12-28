#include <tools/Shell.hpp>
#include <SFML/Window.hpp>
#include <Zenith80.hpp>
#include <string.h>

Shell::Shell(Zenith80 *zenith,tgui::Canvas::Ptr canvas){
	this->zenith = zenith;
	this->canvas = canvas;
	this->history_buffer = new char[4096*256];
	command = zenith->renderText("+-/*:;,<>=?HELLO!abcd^Hello!");
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
