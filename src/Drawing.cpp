#include <Zany80/Drawing.hpp>
#include <Zany80/Zany80.hpp>

#include <cstring>
#include <iostream>

void text(std::string string, int x, int y, sf::Color color) {
	//std::cout << "rendering at "<<x<<", "<<y<<"\n";
	int length = string.length();
	sf::Vertex *vertices = new sf::Vertex[length * 4];
	for (int i = 0; i < length; i++) {
		char c = string[i];
		int cY = c / GLYPHS_PER_ROW, cX = c % GLYPHS_PER_ROW;
		cX *= GLYPH_WIDTH; cY *= GLYPH_HEIGHT;
		vertices[i * 4] = sf::Vertex(sf::Vector2f(x + i * GLYPH_WIDTH,y), color, sf::Vector2f(cX, cY));
		vertices[i * 4 + 1] = sf::Vertex(sf::Vector2f(x + (i + 1) * GLYPH_WIDTH,y), color, sf::Vector2f(cX + GLYPH_WIDTH, cY));
		vertices[i * 4 + 2] = sf::Vertex(sf::Vector2f(x + (i + 1) * GLYPH_WIDTH,y + GLYPH_HEIGHT), color, sf::Vector2f(cX + GLYPH_WIDTH, cY + GLYPH_HEIGHT));
		vertices[i * 4 + 3] = sf::Vertex(sf::Vector2f(x + i * GLYPH_WIDTH,y + GLYPH_HEIGHT), color, sf::Vector2f(cX, cY + GLYPH_HEIGHT));
	}
	window.draw(vertices, length * 4, sf::Quads, &font);
	delete[] vertices;
}
