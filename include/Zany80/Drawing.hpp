#pragma once

#include <map>
#include <string>

#include <SFML/Graphics.hpp>

void text(const char *string, int x, int y);
void cleanupTexts();

extern std::map <std::string,sf::RenderTexture *> texts;
