#include <plugin_manager.hpp>
#include <iostream>

extern std::string folder;

std::vector<std::string> *enumerate_plugins() {
	plugins = new std::vector<std::string>;
	std::cout << "[Plugin Manager] Path: "<<folder<<"plugins/\n";
	plugins->push_back("plugins/cpu/z80");
	plugins->push_back("plugins/rom_runner");
	return plugins;
}

void *cleanup() {
	if  (plugins != nullptr) {
		delete plugins;
		plugins = nullptr;
	}
}

std::vector<std::string> * plugins = nullptr;
