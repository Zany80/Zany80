#include <Zany80/Runner.hpp>

#include <vector>
#include <string>

RunnerType runner_type = Shell;

std::vector<std::string> *history = nullptr;
std::string *command = nullptr;

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *pm) {
	if (history == nullptr) {
		history = new std::vector<std::string>;
	}
	if (command == nullptr) {
		command = new std::string;
	}
}

void cleanup() {
	if (history != nullptr) {
		delete history;
		history = nullptr;
	}
	if (command != nullptr) {
		delete command;
		command = nullptr;
	}
}

void activate() {
	
}

void run() {
	
}

void event(sf::Event &e) {
	switch (e.type) {
		
	}
}
