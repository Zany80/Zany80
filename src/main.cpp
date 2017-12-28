#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <Zany80.hpp>

#include <string.h>
#include <stdlib.h>

int main(){
	Zany80 zany;
	return zany.run();
}

Zany80::Zany80(){
	window = new sf::RenderWindow(sf::VideoMode(LCD_WIDTH,LCD_HEIGHT),"Zany80 IDE");
	gui = new tgui::Gui(*window);
	canvas = tgui::Canvas::create();
	canvas->setSize(LCD_WIDTH,LCD_HEIGHT);
	canvas->setPosition(0,0);
	gui->add(canvas);
	sf::Image f;
	if(!f.loadFromFile("font.png")){
		std::cerr << "Failed to load font!\n";
		exit(1);
	}
	////f.flipVertically();
	if(!font.loadFromImage(f)){
		std::cerr << "Failed to load font!\n";
		exit(1);
	}
	shell = new Shell(canvas);
	tool = shell;
	zany = this;
}

Zany80::~Zany80(){
	delete shell;
	delete gui;
	delete window;
}

int Zany80::run(){
	
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
		canvas->display();
		gui->draw();
		window->display();
	}
}

void Zany80::close(){
	window->close();
}

sf::RenderTexture * Zany80::renderText(const char *string){
	return renderText(string,sf::Color(255,255,255,255));
}

sf::RenderTexture * Zany80::renderText(const char *string,sf::Color fontColor){
	sf::RenderTexture * textImage = new sf::RenderTexture();
	textImage->clear(sf::Color(0,0,0,0));
	int size = strlen(string);
	textImage->create(size * 6 >= LCD_WIDTH ? LCD_WIDTH : size * 6,((size * 6 / LCD_WIDTH) + 1) * 6);
	int x = 0, y = 0;
	for(int i = 0;i < size; i++) {
		char c = string[i];
		sf::IntRect rect((c%16)* 5,(c/16) * 5,5,5);
		sf::Sprite sprite(font,rect);
		sprite.setPosition(x,y);
		sprite.setColor(fontColor);
		textImage->draw(sprite);
		x += 6;
		if ( x > LCD_WIDTH - 6) {
			x = 0;
			y += 6;
		}
	}
	textImage->display();
	return textImage;
}

Zany80 * zany;
