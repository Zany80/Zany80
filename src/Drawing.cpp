#include <Zany80/Drawing.hpp>
#include <Zany80/Zany80.hpp>

#include <cstring>
#include <iostream>

sf::RenderTexture *renderText(const char *string) {
	std::cout << "[Zany80] Rendering string: "<<string<<"\n";
	sf::RenderTexture *texture = new sf::RenderTexture;
	texture->create(LCD_WIDTH,LCD_HEIGHT);
	texture->clear(sf::Color(255,255,255,0));
	int length = strlen(string);
	sf::Sprite font(zany->font);
	for (int i = 0; i < length; i++) {
		char c = string[i] - 32;
		int cY = c / GLYPHS_PER_ROW, cX = c % GLYPHS_PER_ROW;
		font.setTextureRect(sf::IntRect(cX * GLYPH_WIDTH, cY * GLYPH_HEIGHT, GLYPH_WIDTH, GLYPH_HEIGHT));
		font.setPosition(i * GLYPH_WIDTH, 0);
		texture->draw(font);
	}
	texture->display();
	return texture;
}

void text(const char *string, int x, int y) {
	try {
		texts.at(string);
	}
	catch (std::exception) {
		texts[string] = renderText(string);
	}
	sf::Sprite sprite(texts[string]->getTexture());
	sprite.setPosition(x,y);
	zany->window->draw(sprite);
}

void cleanupTexts() {
	for (auto pair : texts) {
		delete pair.second;
	}
	texts.clear();
}

std::map <std::string, sf::RenderTexture *> texts;
