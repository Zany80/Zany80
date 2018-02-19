#include <Zany80/Runner.hpp>
#include <Zany80/Drawing.hpp>
#include <Zany80/Zany80.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <cstring>

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

char *workingDirectory;

typedef struct {
	void(*function)(std::vector<std::string>);
	std::string help;
} command_t;

RunnerType runner_type = Shell;

std::vector<std::string> *history = nullptr;
std::string *command_string = nullptr;

void addToHistory(std::string);

liblib::Library *plugin_manager;

bool displayed = false;

#include "commands.cpp"

void postMessage(PluginMessage m) {
	if (strcmp(m.data, "history") == 0) {
		if (strcmp(m.source, "Runner/ROM") == 0) {
			if (!displayed) {
				displayed = true;
				addToHistory((std::string)m.context);
			}
		}
		else {
			addToHistory((std::string)m.context);
		}
	}
}

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *pm) {
	plugin_manager = pm;
	workingDirectory = nullptr;
	if (history == nullptr) {
		history = new std::vector<std::string>;
	}
	if (command_string == nullptr) {
		command_string = new std::string;
	}

	updateWorkingDirectory();
	
}

void updateWorkingDirectory() {
	if (workingDirectory != nullptr) {
		delete[] workingDirectory;
	}
	workingDirectory = new char[256];
	if (GetCurrentDir(workingDirectory, 256)) {
		int len = strlen(workingDirectory);
		if (len < 256) {
			strcpy(workingDirectory + len, "$ ");
		}
	}
	else {
		delete[] workingDirectory;
		workingDirectory = nullptr;
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

bool activate(const char *arg) {
	return true;
}

void addToHistory(std::string line) {
	#define GLYPHS_PER_LINE (LCD_WIDTH / (GLYPH_WIDTH)) - 1
	// First, make sure every line isn't too big
	if (line.size() <= GLYPHS_PER_LINE) {
		// It fits - most likely situation
		history->push_back(line);
	}
	else {
		while (line.size() > GLYPHS_PER_LINE) {
			history->push_back(line.substr(0,GLYPHS_PER_LINE));
			line = line.substr(GLYPHS_PER_LINE,line.size()-GLYPHS_PER_LINE);
		}
		history->push_back(line);
	}
}

void run() {
	int offset = 0;
	if (workingDirectory != nullptr) {
		text(workingDirectory, 0, LCD_HEIGHT - GLYPH_HEIGHT);
		offset = GLYPH_WIDTH * strlen(workingDirectory);
	}
	text(command_string->c_str(), offset, LCD_HEIGHT - GLYPH_HEIGHT);
	int y = -6;
	for (std::string s : *history) {
		text(s.c_str(), 0, y += GLYPH_HEIGHT);
	}
}

void executeCommand(std::string c) {
	size_t space = c.find_first_of(' ');
	std::vector<std::string> args;
	std::string command = c;
	if (space != std::string::npos) {
		command = command.substr(0, space);
		c = c.substr(space+1,c.size() - space);
		while ((space = c.find_first_of(' ')) != std::string::npos) {
			if (space != 0) {
				args.push_back(c.substr(0,space));
			}
			c.replace(0,space+1,"");
		}
		if (c.size() > 0) {
			args.push_back(c);
		}
	}
	try {
		addToHistory(*command_string);
		command_t _command = commands.at(command);
		try {
			_command.function(args);
		}
		catch (std::exception &e) {
			addToHistory("Error executing command: \"" + command + "\"");
		}
	}
	catch (std::exception) {
		addToHistory("No such command: \"" + command + "\"");
		return;
	}
}

void event(sf::Event &e) {
	switch (e.type) {
		case sf::Event::TextEntered:
			if (e.text.unicode > 127 || e.text.unicode < 32)
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
			else if (e.key.code == sf::Keyboard::BackSpace) {
				if (command_string->size() > 0)
					command_string->pop_back();
			}
			break;
		default:
			break;
	}
}
