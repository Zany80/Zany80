#include <Zany80/GenericException.hpp>
#include <Zany80/Plugins.hpp>
#include <Zany80/Zany80.hpp>

#include <string>

#ifdef _WIN32
int _chdir(const char *);
#define chdir _chdir;
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
			.help = "Echoes all received arguments to the history buffer.\n",
			.detailed_help = "Run `echo Hello there, self!` to see it in action."
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
			}
			catch (GenericException e) {
				addToHistory(e.what());
			}
			catch (std::exception) {
				addToHistory("Unable to run ROM!");
			}
		},
		.help = "Launches the specified cart."
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
	}},

	{"assemble", {
		.function = [](std::vector<std::string> args) {
			((message_t)(*plugin_manager)["message"])({
				0, "invoke", (int)strlen("invoke"), "Runner/Shell", (char *)&args
			}, "Assembler/z80");
		},
		.help = "Invokes the assembler to turn an assembly input file into an object file."
	}},
	
	{"compile", {
		.function = [](std::vector<std::string> args) {
			args.push_back("-c -I");
			args.push_back(true_folder + "/libc/include");
			args.push_back("-Wp,-include -Wp,stdbool.h");
			args.push_back("-Wp,-include -Wp,output.h");
			args.push_back("-Wp,-include -Wp,input.h");
			args.push_back("-Wp,-include -Wp,system.h");
			((message_t)(*plugin_manager)["message"])({
				0, "invoke", (int)strlen("invoke"), "Runner/Shell", (char *)&args
			}, "CCompiler/z80");
		},
		.help = "Invokes the C compiler."
	}},
	
	{"link", {
		.function = [](std::vector<std::string> args) {
			((message_t)(*plugin_manager)["message"])({
				1, "invoke", (int)strlen("invoke"), "Runner/Shell", (char *)&args
			}, "Linker/z80");
		},
		.help = "Invokes the linker to turn object files into a usable ROM"
	}},
	
	{"clear", {
		.function = [](std::vector<std::string> args) {
			history->clear();
		},
		.help = "Clears the shell display"
	}},

	{"help", {
		.function = [](std::vector<std::string> args) {
			if (args.size() == 0) {
				addToHistory("Commands: ");
				for (auto pair : commands) {
					std::string command_info = "\t" + pair.first + ": "+pair.second.help;
					addToHistory(command_info);
				}
			}
			else if (args.size() == 1) {
				auto command = commands.find(args[0]);
				if (command != commands.end()) {
					std::string help = command->second.detailed_help;
					if (help == "") {
						addToHistory("No help available for that command!");
					}
					else {
						addToHistory(help);
					}
				}
				else {
					addToHistory("No such command!");
				}
			}
			else {
				addToHistory("Usage: `help [command]`");
			}
		},
		.help = "Prints out information on commands. For more information, run `help help`",
		.detailed_help = "Prints out information on commands. With no arguments, gives general help. "
		"If run with the name of a command (e.g. `help run`), gives more detailed information."
	}}
	
};
