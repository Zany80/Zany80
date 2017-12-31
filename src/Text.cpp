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

void Text::updateTexture(){
	if (texture != nullptr) {
		delete texture;
	}
	texture = zany->renderText(text.c_str());
}

const sf::Texture * Text::getTexture(){
	return &this->texture->getTexture();
}

void Text::setString(std::string string){
	text = string;
	updateTexture();
}

void Text::append(std::string string){
	text += string;
	updateTexture();
}

void Text::append(char c){
	text.push_back(c);
	updateTexture();
}

std::string * Text::getString(){
	return &this->text;
}

Text::~Text(){
	if (this->texture != nullptr) {
		delete texture;
	}
}
