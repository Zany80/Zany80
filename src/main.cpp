#include <SFML/Graphics.hpp>
#include <Zany80.hpp>

#include <string.h>
#include <stdlib.h>

#include <sstream>
#include <iostream>

int main(){
	zany = new Zany80();
	return zany->run();
}

Zany80::Zany80(){
	window = new sf::RenderWindow(sf::VideoMode(LCD_WIDTH,LCD_HEIGHT),"Zany80 IDE");
	if(!font.loadFromFile("font.png")){
		std::cerr << "Failed to load font!\n";
		delete window;
		exit(1);
	}
	tool = nullptr;
}

Zany80::~Zany80(){
	delete window;
}

int Zany80::run(){
	while (window->isOpen()) {
		frame();
	}
}

void Zany80::frame(){
	sf::Event e;
	while (window->pollEvent(e)) {
		switch (e.type) {
			case sf::Event::Closed:
				close();
				break;
			default:
				if (tool != nullptr)
					tool->event(e);
				break;
		}
	}
	if (tool != nullptr)
		tool->run();
	window->display();
}

void Zany80::close(){
	window->close();
}

Zany80 * zany;
