#include <Zany80/Runner.hpp>

#include <vector>
#include <map>
#include <iostream>

typedef void(*command_t)(std::vector<sf::String>);

RunnerType runner_type = Shell;

std::vector<sf::String> *history = nullptr;
sf::String *command_string = nullptr;

std::map <sf::String, command_t> commands;

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *pm) {
	if (history == nullptr) {
		history = new std::vector<sf::String>;
	}
	if (command_string == nullptr) {
		command_string = new sf::String;
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

void run() {
	
}

void executeCommand(sf::String c) {
	size_t space = c.find(" ");
	std::vector<sf::String> args;
	sf::String command = c;
	if (space != sf::String::InvalidPos) {
		command = command.substring(0, space);
		c.replace(0,space+1,"");
		while ((space = c.find(" ")) != sf::String::InvalidPos) {
			if (space != 0) {
				args.push_back(c.substring(0,space));
			}
			c.replace(0,space+1,"");
		}
	}
	try {
		commands.at(command)(args);
	}
	catch (std::exception) {
		std::cerr << "Unable to execute command \""<<command.toAnsiString()<<"\"...\n";
		// No such command_string
		history->push_back("");
	}
}

void event(sf::Event &e) {
	switch (e.type) {
		case sf::Event::TextEntered:
			switch (e.text.unicode) {
				case 13: // CR - in case on some platforms this *isn't* the enter key, use the KeyPressed event for enter instead.
					break;
				default:
					*command_string += e.text.unicode;
					std::cout << "Character typed: "<<e.text.unicode<<"\n";
					break;
			}
			break;
		case sf::Event::KeyPressed:
			if (e.key.code == sf::Keyboard::Return) {
				executeCommand(*command_string);
				command_string->clear();
			}
			break;
		default:
			break;
	}
}
