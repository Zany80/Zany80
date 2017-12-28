#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>

class Text{
public:
	Text();
	Text(const char *string);
	~Text();
	void setString(const char *string);
	const char *getString();
	void append(const char *string);
	void append(char c);
	void updateTexture();
	sf::Texture getTexture();
private:
	sf::Mutex textureMutex;
	sf::RenderTexture * texture;
	std::string text;
};
