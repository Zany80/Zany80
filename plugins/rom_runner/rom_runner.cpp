#include <Zany80/Runner.hpp>
#include <string>
#include <iostream>
#include <fstream>

#include <SFML/System.hpp>

RunnerType runner_type = ROMRunner;
liblib::Library *z80;

uint8_t * ROM = nullptr;

const char *neededPlugins() {
	return "CPU/z80";
}

void init(liblib::Library *plugin_manager) {
	std::cout << "[ROM Runner] Retrieving z80 plugin...\n";
	try {
		z80 = ((liblib::Library*(*)(const char *))((*plugin_manager)["getCPU"]))("z80");
		if (z80 == nullptr) {
			throw std::exception();
		}
	}
	catch (liblib::SymbolLoadingException) {
		throw std::exception();
	}
}

void loadROM(std::string path) {
	if (ROM != nullptr) {
		delete[] ROM;
	}
	std::ifstream ROMFile(path,std::ios::binary | std::ios::ate);
	int size = ROMFile.tellg();
	ROM = new uint8_t[size];
	ROMFile.seekg(0);
	ROMFile.read((char*)ROM,size);
}

void cleanup() {
	if (ROM != nullptr) {
		delete[] ROM;
	}
}

sf::Clock timer;

void run() {
	((void(*)(uint64_t))((*z80)["emulate"]))(4000000 * timer.restart().asSeconds());
}

void activate() {
	timer.restart();
}
