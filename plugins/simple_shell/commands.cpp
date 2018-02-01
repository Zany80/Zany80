#include <exception>

std::map <sf::String, command_t> commands = {
	
	{
		"echo", {
			.function = [](std::vector<std::string> args){
				if (history != nullptr) {
					for (std::string s: args) {
						addToHistory(s);
					}
				}
				else {
					throw std::exception();
				}
			},
			.help = "Echoes all received arguments to the history buffer.\n"
		}
	}
	
};
