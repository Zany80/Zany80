#include <Text.hpp>
#include <string.h>

#include <Zany80.hpp>

Text::Text(){
	texture = nullptr;
	setString("");
}

Text::Text(const char *string){
	texture = nullptr;
	setString(string);
}

sf::Texture Text::getTexture(){
	return this->texture->getTexture();
}

void Text::setString(const char *string){
	text = string;
	if (texture != nullptr) {
		delete texture;
	}
	texture = zany->renderText(text.c_str());
}

void Text::append(const char *string){
	text += string;
	if (texture != nullptr) {
		delete texture;
	}
	texture = zany->renderText(text.c_str());
}

const char *Text::getString(){
	return this->text.c_str();
}

void Text::append(char c){
	text += c;
	if (texture != nullptr) {
		delete texture;
	}
	texture = zany->renderText(text.c_str());
}

Text::~Text(){
	if (this->texture != nullptr) {
		delete texture;
	}
}
