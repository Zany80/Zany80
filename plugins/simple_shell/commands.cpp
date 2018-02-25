#include <Zany80/GenericException.hpp>
#include <Zany80/Plugins.hpp>

#include <string>

#ifdef _WIN32
int _chdir(const char *);
using chdir = _chdir;
#else
int chdir(const char *);
#endif

extern void updateWorkingDirectory();

std::map <std::string, command_t> commands = {
	
	{"echo", {
			.function = [](std::vector<std::string> args){
				std::string all_args;
				for (std::string s: args) {
					all_args += s + " ";
				}
				all_args.pop_back();
				addToHistory(all_args);
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
				if (((bool(*)(RunnerType r,const char *arg))((*plugin_manager)["activateRunner"]))(ROMRunner,args[0].c_str())) {
					// ROM run successfully
					addToHistory("ROM running, control ceded.");
				}
				else {
					displayed = false;
				}
			}
			catch (GenericException e) {
				addToHistory(e.what());
			}
			catch (std::exception) {
				addToHistory("Unable to run ROM!");
			}
		}
	}},

	{"cd", {
		.function = [](std::vector<std::string> args) {
			if (args.size() != 1) {
				addToHistory("Usage: `cd directory`");
				return;
			}
			chdir(args[0].c_str());
			updateWorkingDirectory();
		},
		.help = "Changes the current directory"
	}}
	
};
