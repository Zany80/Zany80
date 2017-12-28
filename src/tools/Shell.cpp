#include <tools/Shell.hpp>
#include <Zany80.hpp>
#include <string.h>

Shell::Shell(tgui::Canvas::Ptr canvas){
	this->canvas = canvas;
}

Shell::~Shell(){
	
}

void Shell::run(){
	canvas->clear(sf::Color(0,0,0));
	sf::Sprite s(command.getTexture());
	s.setPosition(0,LCD_HEIGHT-s.getGlobalBounds().height);
	canvas->draw(s);
}

void Shell::event(sf::Event e){
	switch(e.type){
		case sf::Event::TextEntered:
			if (e.text.unicode < 128) {
				command.append((char)e.text.unicode);
			}
			std::cout << command.getString()<<"\n";
			break;
	}
}
