#pragma once

#include <map>
#include <string>

#include <SFML/Graphics.hpp>

void text(std::string string, int x, int y);

class Text : public sf::Drawable {
public:
	Text(const char *string);
	Text(const char *string, int x, int y);
	~Text();
private:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
	sf::Vertex * vertices;
	const int length;
};
