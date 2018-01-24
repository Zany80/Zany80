#include <Zany80/Runner.hpp>
#include <string>
#include <iostream>
#include <fstream>

#include <SFML/System.hpp>

#define SPEED 4000000

RunnerType runner_type = ROMRunner;
liblib::Library *z80;
void (*emulate)(uint64_t cycles);

uint8_t * ROM = nullptr;

float to_run;
sf::Clock timer, precision;

const char *neededPlugins() {
	return "CPU/z80";
}

void init(liblib::Library *plugin_manager) {
	to_run = 0;
	std::cout << "[ROM Runner] Retrieving z80 plugin...\n";
	try {
		z80 = ((liblib::Library*(*)(const char *))((*plugin_manager)["getCPU"]))("z80");
		if (z80 == nullptr) {
			throw std::exception();
		}
		emulate=(void(*)(uint64_t))((*z80)["emulate"]);
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
	float time_passed = precision.getElapsedTime().asSeconds();
	if (z80 == nullptr)
		return;
	if (ROM != nullptr) {
		delete[] ROM;
	}
	uint64_t cycles = (float)*(uint64_t*)((*z80)["getCycles"]());
	uint64_t target = SPEED * time_passed;
	std::cout << "[Rom Runner] After "<<time_passed<<" seconds: Cycles: "<<cycles<<"; Target: "<<target<<"\n";
	std::cout << "[ROM Runner] CCA: "<<100*(double)cycles/(double)target<< "%\n";
	z80 = nullptr;
}

void run() {
	to_run += SPEED * timer.restart().asSeconds();
	emulate((uint64_t)to_run);
	to_run -= (uint64_t)to_run;
}

void activate() {
	timer.restart();
	precision.restart();
}
