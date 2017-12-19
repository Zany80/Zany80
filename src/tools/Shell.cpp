#include <tools/Shell.hpp>
#include <SFML/Window.hpp>
#include <Zenith80.hpp>

Shell::Shell(Zenith80 *zenith,tgui::Canvas::Ptr canvas){
	this->zenith = zenith;
	this->canvas = canvas;
}

Shell::~Shell(){
	
}

void Shell::run(){
	canvas->clear(sf::Color(0,0,0));
}

void Shell::event(sf::Event e){
	
}
