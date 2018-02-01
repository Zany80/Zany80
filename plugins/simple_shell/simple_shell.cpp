#include <Zany80/Runner.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <string>

typedef struct {
	void(*function)(std::vector<std::string>);
	std::string help;
} command_t;

RunnerType runner_type = Shell;

std::vector<std::string> *history = nullptr;
std::string *command_string = nullptr;

void addToHistory(std::string);

#include "commands.cpp"

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *pm) {
	if (history == nullptr) {
		history = new std::vector<std::string>;
	}
	if (command_string == nullptr) {
		command_string = new std::string;
	}
}

void cleanup() {
	if (history != nullptr) {
		delete history;
		history = nullptr;
	}
	if (command_string != nullptr) {
		delete command_string;
		command_string = nullptr;
	}
}

void activate() {
	
}

void addToHistory(std::string line) {
	// First, make sure every line isn't too big
	if (line.size() <= (LCD_WIDTH / (FONT_WIDTH)) - 1) {
		// It fits - most likely situation
		history->push_back(line);
	}
	else {
		while (line.size() > 40) {
			history->push_back(line.substr(0,40));
			line = line.substr(40,line.size()-40);
		}
		history->push_back(line);
	}
}

void run() {
	
}

void executeCommand(std::string c) {
	size_t space = c.find(" ");
	std::vector<std::string> args;
	std::string command = c;
	if (space != std::string::npos) {
		command = command.substr(0, space);
		c.replace(0,space+1,"");
		while ((space = c.find(" ")) != std::string::npos) {
			if (space != 0) {
				args.push_back(c.substr(0,space));
			}
			c.replace(0,space+1,"");
		}
	}
	try {
		commands.at(command).function(args);
	}
	catch (std::exception) {
		std::cerr << "Unable to execute command \""<<command<<"\"\n";
		// No such command_string
		addToHistory("No such command: \"" + command + "\"");
	}
}

void event(sf::Event &e) {
	switch (e.type) {
		case sf::Event::TextEntered:
			if (e.text.unicode > 127)
				break;
			switch (e.text.unicode) {
				case 13: // CR - in case on some platforms this *isn't* the enter key, use the KeyPressed event for enter instead.
					break;
				default:
					*command_string += e.text.unicode;
					break;
			}
			break;
		case sf::Event::KeyPressed:
			if (e.key.code == sf::Keyboard::Return) {
				if (command_string->size() > 0) {
					executeCommand(*command_string);
				}
				command_string->clear();
			}
			break;
		default:
			break;
	}
}
