#include <exception>

std::map <sf::String, command_t> commands = {
	
	{
		"echo", {
			.function = [](std::vector<std::string> args){
				for (std::string s: args) {
					addToHistory(s);
				}
			},
			.help = "Echoes all received arguments to the history buffer.\n"
		}
	}
	
};
