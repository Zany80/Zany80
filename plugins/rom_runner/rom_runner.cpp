#include <Zany80/Runner.hpp>
#include <string>
#include <iostream>

RunnerType runner_type = ROM;
liblib::Library *z80;

const char *neededPlugins() {
	return "CPU/z80";
}

void init(liblib::Library *plugin_manager) {
	std::cout << "[ROM Runner] Retrieving z80 plugin...\n";
	try {
		z80 = ((liblib::Library*(*)(const char *))((*plugin_manager)["getCPU"]))("z80");
		if (z80 == nullptr) {
			throw 3;
		}
	}
	catch (liblib::SymbolLoadingException) {
		throw std::exception();
	}
}

void loadROM(std::string path) {
	
}

void cleanup() {
	
}

void run() {
	
}
