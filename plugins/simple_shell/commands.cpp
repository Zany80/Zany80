#include <exception>
#include <Zany80/Plugins.hpp>

std::map <sf::String, command_t> commands = {
	
	{"echo", {
			.function = [](std::vector<std::string> args){
				for (std::string s: args) {
					addToHistory(s);
				}
			},
			.help = "Echoes all received arguments to the history buffer.\n"
	}},
	{"run", {
		.function = [](std::vector<std::string> args) {
			if (args.size() == 0){
				addToHistory("No ROM specified!");
				return;
			}
			try {
				((void(*)(RunnerType r,const char *arg))((*plugin_manager)["activateRunner"]))(ROMRunner,args[0].c_str());
			}
			catch (std::exception) {
				addToHistory("Unable to run ROM!");
			}
		}
	}}
	
};
