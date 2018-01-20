#include <tools/Shell.hpp>
#include <Zany80.hpp>
#include <string.h>
#include <Drawing.hpp>

Shell::Shell(){
	map_commands();
}

Shell::~Shell(){
	
}


inline void Shell::map_commands(){
	commands["echo"] = [](Shell *shell,std::vector<std::string> arguments){
		std::string sum_args = "";
		for (std::string arg : arguments) {
			sum_args += arg + ' ';
		}
		sum_args.pop_back();
		shell->addToHistory(sum_args+"\n");
	};
	help["echo"] = "Echoes all arguments passed to it.\n";
	commands["exit"] = [](Shell *shell,std::vector<std::string> arguments){
		zany->close();
	};
	help["exit"] = "Closes Zany80. This will *not* save any data.\n";
	commands["help"] = [](Shell *shell,std::vector<std::string> arguments){
		if (arguments.size() == 1) {
			if (shell->help.find(arguments[0]) != shell->help.end()) {
				shell->addToHistory(shell->help[arguments[0]]);
			}
			else {
				shell->addToHistory(arguments[0] + "is not a command.\n");
			}
		}
		else {
			shell->addToHistory(
				"Commands:\n\n"
				"help\n"
				"\tPrints out list of commands\n"
				"echo [arg1] [arg2] [argn]\n"
				"\tEchoes all arguments given\n"
				"exit\n"
				"\tCloses Zany80\n"
				"color r g b\n"
				"\tSets color\n"
				"clear\n"
				"\tClears the history"
				"\n"
			);
		}
	};
	help["help"] = "Prints out helpful information about commands. Use with no arguments for a command overview.\n";
	commands["color"] = [](Shell *shell,std::vector<std::string> arguments){
		if (arguments.size() == 3) {
			shell->setColor(Color(std::stoi(arguments[0]),std::stoi(arguments[1]),std::stoi(arguments[2])));
			shell->history.updateSprite();
		}
		else {
			shell->addToHistory(
				"\nUsage:\n"
				"\tcolor r g b\n"
				"Use `help color` for more information.\n\n"
			);
		}
	};
	help["color"] = "Sets the font color of the shell to the color provided. "
	"Color is given in RGB format. For example, to set the color to red, use `color 255 0 0`. "
	"If you don't know how RGB works, you can look up codes with Google.\n";
	commands["clear"] = [](Shell *shell,std::vector<std::string> arguments){
		shell->history.setString("");
	};
	help["clear"] = "Clears the screen.";
}

void Shell::setColor(Color color){
	command.setColor(color);
	history.setColor(color);
}

void Shell::run(){
	
	Zany::clear(sf::Color(3,220,100));
	Zany::draw(*(command.getSprite()),0,LCD_HEIGHT - (*command.getSprite()).getHeight());
	
	////sf::Sprite commandSprite(*command.getTexture());
	////commandSprite.setPosition(0,LCD_HEIGHT - commandSprite.getGlobalBounds().height);
	////window->draw(commandSprite);

	////if (history.getTexture()->getSize().y > (LCD_HEIGHT - 12)) {
		////history.setString(history.getString()->substr(history.getString()->find_first_of('\n') + 1));
	////}
	
	////sf::Sprite historySprite(*history.getTexture());
	////historySprite.setPosition(0,0);
	////window->draw(historySprite);
	
}

void Shell::event(sf::Event e){
	switch(e.type){
		case sf::Event::TextEntered:
			if (e.text.unicode < 128) {
				if (e.text.unicode == 0x0D) {
					// 'Enter' hit, execute command
					std::string to_execute;
					std::vector<std::string> args;
					size_t space = command.getString()->find_first_of(' ');
					if (space == std::string::npos) {
						// if no space found, command is entire string
						to_execute = *command.getString();
					}
					else {
						// command is up to first space
						to_execute = command.getString()->substr(0,space);
						std::string remaining = command.getString()->substr(space+1,command.getString()->size()-space);
						while ((space = remaining.find_first_of(' ')) != std::string::npos) {
							args.push_back(remaining.substr(0,space));
							remaining = remaining.substr(space+1,remaining.size()-space);
						}
						args.push_back(remaining);
					}
					addToHistory(*command.getString()+"\n");
					command.setString("");
					if (commands.find(to_execute) != commands.end()) {
						commands[to_execute](this,args);
					}
					else {
						addToHistory("Error: `" + to_execute + "` command unrecognized!\n");
					}
				}
				else if (e.text.unicode == 0x08) {
					// Backspace, remove last character
					if (command.getString()->size() > 0) {
						command.getString()->pop_back();
					}
					command.updateSprite();
				}
				else {
					////std::cout<<"code: "<<(int)e.text.unicode<<"\n";
					command.append((char)e.text.unicode);
				}
			}
			break;
	}
}

void Shell::addToHistory(std::string history_item){
	history.append(history_item);
}
