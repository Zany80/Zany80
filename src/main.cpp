#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <Zenith80.hpp>

int main(){
	Zenith80 zenith;
	return zenith.run();
}

Zenith80::Zenith80(){
	window = new sf::RenderWindow(sf::VideoMode(800,600),"Zenith80 IDE");
	gui = new tgui::Gui(*window);
	canvas = tgui::Canvas::create();
	canvas->setSize("100%","100%");
	canvas->setPosition(0,0);
	gui->add(canvas);
	shell = new Shell(this,canvas);
	tool = shell;
}

Zenith80::~Zenith80(){
	delete shell;
	delete gui;
	delete window;
}

int Zenith80::run(){
	
	while (window->isOpen()) {
		window->clear(sf::Color(255,255,255));
		sf::Event e;
		while (window->pollEvent(e)) {
			switch (e.type) {
				case sf::Event::Closed:
					window->close();
					break;
				default:
					tool->event(e);
					break;
			}
		}
		
		tool->run();
		gui->draw();
		window->display();
	}
}

void Zenith80::close(){
	window->close();
}
