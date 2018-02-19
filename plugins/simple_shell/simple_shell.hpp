#pragma once

#include <string>
#include <vector>
#include <liblib/liblib.hpp>

void addToHistory(std::string);

typedef struct {
	void(*function)(std::vector<std::string>);
	std::string help;
} command_t;

extern liblib::Library *plugin_manager;

extern bool displayed;
