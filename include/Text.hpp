#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>

class Text{
public:
	Text();
	Text(const char *string);
	~Text();
	void setString(std::string string);
	std::string *getString();
	void append(std::string string);
	void append(char c);
	void updateTexture();
	const sf::Texture * getTexture();
private:
	sf::Mutex textureMutex;
	sf::RenderTexture * texture;
	std::string text;
};
