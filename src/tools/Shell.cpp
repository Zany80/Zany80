#include <tools/Shell.hpp>
#include <Zany80.hpp>
#include <string.h>

Shell::Shell(tgui::Canvas::Ptr canvas){
	this->canvas = canvas;
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
	commands["exit"] = [](Shell *shell,std::vector<std::string> arguments){
		zany->close();
	};
	commands["help"] = [](Shell *shell,std::vector<std::string> arguments){
		shell->addToHistory(
			"Commands:\n\n"
			"help\n"
			"\tPrints out list of commands\n"
			"echo [arg1] [arg2] [argn]\n"
			"\tEchoes all arguments given\n"
			"exit\n"
			"\tCloses Zany80\n"
			"\n"
		);
	};
}

void Shell::run(){
	canvas->clear(sf::Color(0,0,0));
	
	sf::Sprite command_sprite(*command.getTexture());
	command_sprite.setPosition(0,LCD_HEIGHT-command_sprite.getGlobalBounds().height);
	canvas->draw(command_sprite);
	
	sf::Sprite history_sprite(*history.getTexture(),sf::IntRect(0,0,LCD_WIDTH,LCD_HEIGHT-command_sprite.getGlobalBounds().height));
	history_sprite.setPosition(0,0);
	canvas->draw(history_sprite);
	
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
						// "echo a"
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
				}
				else if (e.text.unicode == 0x08) {
					// Backspace, remove last character
					if (command.getString()->size() > 0) {
						command.getString()->pop_back();
					}
					command.updateTexture();
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
