#pragma once

#include <SFML/Window.hpp>

class Tool {
public:
	virtual void run() = 0;
	virtual void event(sf::Event e) = 0;
};
