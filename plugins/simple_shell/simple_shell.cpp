#define NEEDED_PLUGINS ""

#include <Zany80/Plugins/Runner.hpp>
#include <Zany80/Drawing.hpp>
#include <Zany80/Zany80.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <cstring>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

bool isCategory(const char *sig) {
	return !strcmp(sig, "Runner") || !strcmp(sig, "Shell");
}

bool isType(const char *sig) {
	return !strcmp(sig, "Shell");
}

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

#define GLYPHS_PER_LINE (LCD_WIDTH / (GLYPH_WIDTH))

int scroll_up;

std::string workingDirectory, command_line;
int command_lines;

std::string *command_string = nullptr;
std::vector<std::string> *history = nullptr;

void updateCommandLine() {
	command_line = workingDirectory + "$ " + *command_string;
	int pcommand_lines = command_lines;
	command_lines = command_line.size() / GLYPHS_PER_LINE;
	if (command_line.size() % GLYPHS_PER_LINE == 0) {
		command_lines--;
	}
	if (command_lines > pcommand_lines) {
		scroll_up += GLYPH_HEIGHT;
	}
	else if (command_lines < pcommand_lines) {
		scroll_up -= GLYPH_HEIGHT;
	}
}

typedef struct {
	void(*function)(std::vector<std::string>);
	std::string help;
	std::string detailed_help;
} command_t;

void addToHistory(std::string);

liblib::Library *plugin_manager;

#include "commands.cpp"

std::map<std::string, std::string(*)(std::string)> autocompletion_map = {
	{"help", [](std::string last_word) {
		std::vector<std::string> possible_commands;
		for (auto pair : commands) {
			if (pair.first.compare(0, last_word.size(), last_word) == 0
					&& pair.second.detailed_help.size()) {
				possible_commands.push_back(pair.first);
			}
		}
		if (possible_commands.size() == 1) {
			return possible_commands[0] + ' ';
		}
		if (possible_commands.size()) {
			std::string c;
			for (std::string s : possible_commands) {
				c += s + " ";
			}
			c = c.substr(0, c.size() - 1);
			addToHistory(c);
		}
		return last_word;
	}}
};

RunnerType runner_type = Shell;

void postMessage(PluginMessage m) {
	if (strcmp(m.data, "history") == 0) {
		if (strcmp(m.source, "Runner/ROM") == 0) {
			addToHistory((std::string)m.context);
		}
		else {
			addToHistory((std::string)m.context);
		}
	}
	else if (strcmp(m.data, "init") == 0) {
		plugin_manager = (liblib::Library*)m.context;
		workingDirectory = "";
		if (history == nullptr) {
			history = new std::vector<std::string>;
			addToHistory("Zany80 Simple Shell...");
			addToHistory("Hello! For help, contact me at pleasantatk@gmail.com");
			addToHistory("You can scroll using CTRL+UP and CTRL+DOWN");
			addToHistory("Also, for help, you can use `help`. It's an extremely helpful command ;)");
		}
		if (command_string == nullptr) {
			command_string = new std::string;
		}
		scroll_up = 0;
		updateWorkingDirectory();
	}
	else if (strcmp(m.data, "cleanup") == 0) {
		if (history != nullptr) {
			delete history;
			history = nullptr;
		}
		if (command_string != nullptr) {
			delete command_string;
			command_string = nullptr;
		}
	}
}

void updateWorkingDirectory() {
	char _workingDirectory[FILENAME_MAX];
	if (GetCurrentDir(_workingDirectory, FILENAME_MAX)) {
		workingDirectory = _workingDirectory;
	}
	else {
		workingDirectory = "";
	}
	updateCommandLine();
}

bool activate(const char *arg) {
	return true;
}
	
void addToHistory(std::string line) {
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

const sf::Color bg(30, 30, 30);
const sf::Color text_color(225, 225, 225);

void run() {
	clear(bg);
	int y = LCD_HEIGHT - (command_lines + 2) * GLYPH_HEIGHT;
	for (int i = 0; i <= command_lines; i++) {
		text(command_line.substr(i * GLYPHS_PER_LINE, i * GLYPHS_PER_LINE + GLYPHS_PER_LINE), 0, y += GLYPH_HEIGHT, text_color);
	}
	for (int i = history->size() -1; i > 0;i--) {
		if ((y -= GLYPH_HEIGHT) - scroll_up > (LCD_HEIGHT - GLYPH_HEIGHT * 2))
			continue;
		text((*history)[i].c_str(), 0, y - scroll_up, text_color);
		if (y - scroll_up < 0)
			break;
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
		addToHistory(command_line);
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

std::string default_autocompleter(std::string word) {
	std::vector<std::string> files;
	for (fs::directory_entry entry : fs::directory_iterator(workingDirectory)) {
		std::string path = entry.path();
		path = path.substr(path.find_last_of('/') + 1);
		if (path.compare(0, word.size(), word) == 0) {
			files.push_back(path);
		}
	}
	if (files.size() == 1) {
		return files[0] + ' ';
	}
	else if (files.size()) {
		std::string s;
		for (std::string f : files) {
			s += f + ' ';
		}
		addToHistory(s.substr(0, s.size() - 1));
	}
	return word;
}

void autocomplete() {
	std::string current_command = *command_string;
	// Strip beginning whitespace
	while (isspace(current_command[0])) {
		current_command = current_command.substr(1, current_command.size() - 1);
	}
	// Find end of first word
	uint i;
	for (i = 0; i < current_command.size() && !isspace(current_command[i]); i++);
	std::string first_word = current_command.substr(0, i);
	if (i == current_command.size()) {
		// Only one word, treat it as a command
		std::vector<std::string> possible_commands;
		for (auto pair : commands) {
			if (pair.first.compare(0, first_word.size(), first_word) == 0) {
				possible_commands.push_back(pair.first);
			}
		}
		if (possible_commands.size() == 1) {
			*command_string = possible_commands[0] + ' ';
		}
		else if (possible_commands.size()) {
			std::string c;
			for (std::string s : possible_commands) {
				c += s + " ";
			}
			c = c.substr(0, c.size() - 1);
			addToHistory(c);
		}
	}
	else {
		// Get last argument
		std::string last_word = " ";
		for (i = current_command.size(); i > 0; i--) {
			if (isspace(current_command[i])) {
				last_word = current_command.substr(i + 1, current_command.size() - i);
				break;
			}
		}
		std::string(*autocompleter)(std::string) = default_autocompleter;
		try {
			autocompleter = autocompletion_map.at(first_word);
			if (autocompleter == nullptr)
				autocompleter = default_autocompleter;
		}
		catch (std::exception &e) {
			autocompleter = default_autocompleter;
		}
		// autocomplete last word
		for (i = command_string->size(); i > 0 && !isspace((*command_string)[i]); i--);
		*command_string = command_string->substr(0, i + 1) + autocompleter(last_word);
	}
	updateCommandLine();
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
					updateCommandLine();
					break;
			}
			break;
		case sf::Event::KeyPressed:
			if (e.key.code == sf::Keyboard::Return) {
				if (command_string->size() > 0) {
					executeCommand(*command_string);
				}
				command_string->clear();
				updateCommandLine();
			}
			if (e.key.code == sf::Keyboard::BackSpace) {
				if (command_string->size() > 0) {
					command_string->pop_back();
					updateCommandLine();
				}
			}
			if (e.key.code == sf::Keyboard::Tab) {
				autocomplete();
			}
			if (e.key.control && e.key.code == sf::Keyboard::Up) {
				scroll_up += 5;
			}
			if (e.key.control && e.key.code == sf::Keyboard::Down) {
				scroll_up -= 5;
			}
			if (e.key.control && e.key.code == sf::Keyboard::Equal) {
				scroll_up = 0;
			}
			break;
		default:
			break;
	}
}
