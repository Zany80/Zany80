#include <Zany80/GenericException.hpp>
#include <Zany80/Plugins.hpp>
#include <Zany80/Zany80.hpp>

#include <string>

#ifdef _WIN32
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
			.detailed_help = "Echoes all received arguments to the history buffer.\n"
							 "Run `echo Hello there, self!` to see it in action."
	}},

	{"load", {
		.function = [](std::vector<std::string> args) {
			if (args.size() == 2) {
				int target;
				if (args[1].size() == 1) {
					if (args[1][0] >= '0' && args[1][0] <= '9') {
						((message_t)(*plugin_manager)["message"])({
							args[1][0] - '0', "load_cart", (int)strlen("load_cart"), "Runner/Shell", args[0].c_str()
						}, "Hardware/CartridgeManager");
					}
					else {
						addToHistory(args[1] + " is not a valid number!");
					}
				}
				else {
					addToHistory(args[1] + " is not a valid number!");
				}
			}
			else if (args.size()) {
				((message_t)(*plugin_manager)["message"])({
					0, "load_cart", (int)strlen("load_cart"), "Runner/Shell", args[0].c_str()
				}, "Hardware/CartridgeManager");
			}
			else {
				addToHistory("Usage: `load file.rom [slot]`");
			}
		},
		.help = "Loads a cartridge",
		.detailed_help =
		"With 1 argument, loads into slot 0. (e.g. `load a.rom`). "
		"With 2 arguments, loads into the specified slot (e.g. `load a.rom 3). "
		"In the second form, only 1-digit numbers are valid for argument 2."
	}},
	
	{"run", {
		.function = [](std::vector<std::string> args) {
			try {
				if (((bool(*)(RunnerType r,const char *arg))((*plugin_manager)["activateRunner"]))(ROMRunner,"")) {
					// ROM run successfully
					addToHistory("ROM running, control ceded.");
				}
			}
			catch (GenericException e) {
				addToHistory(e.what());
			}
			catch (...) {
				addToHistory("Unable to run ROM!");
			}
		},
		.help = "Launches the specified cart."
	}},

	{"reset", {
		.function = [](std::vector<std::string> args) {
			((textMessage_t)(*plugin_manager)["textMessage"])("reset", "Runner/Shell;CPU/z80");
			((textMessage_t)(*plugin_manager)["textMessage"])("reset", "Runner/Shell;Runner/ROM");
		},
		.help = "Reset the CPU",
		.detailed_help = "Resets the CPU. This restarts any game that is open."
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
			args.push_back("-c");
			((message_t)(*plugin_manager)["message"])({
				0, "invoke", (int)strlen("invoke"), "Runner/Shell", (char *)&args
			}, "Assembler/z80");
		},
		.help = "Invokes the assembler to turn an assembly input file into an object file."
	}},
	
	{"compile", {
		.function = [](std::vector<std::string> args) {
			args.push_back("-c");
			args.push_back("-I");
			args.push_back(folder + "/include/Zany80/libc/");
			args.push_back("-Wp,-include -Wp,system.h");
			args.push_back("-Wp,-include -Wp,output.h");
			args.push_back("-Wp,-include -Wp,input.h");
			((message_t)(*plugin_manager)["message"])({
				0, "invoke", (int)strlen("invoke"), "Runner/Shell", (char *)&args
			}, "CCompiler/z80");
		},
		.help = "Invokes the C compiler."
	}},
	
	{"link", {
		.function = [](std::vector<std::string> args) {
			// WARNING: The following two for loops should *not* be
			// condensed. This is to ensure that the template.o file
			// is added *before* the libc! (the libc is pushed to the
			// front first, then the template is pushed to the front,
			// resulting in proper linkage)
			for (int i = 0; i < args.size(); i++) {
				if (args[i] == "-c" || args[i] == "--link-crt") {
					args.push_back("");
					args.erase(args.begin() + i);
					for (int i = args.size() - 1; i >= 0; i--) {
						args[i+1] = args[i];
					}
					args[0] = folder + "/libc.o";
				}
			}
			for (int i = 0; i < args.size(); i++) {
				if (args[i] == "-e" || args[i] == "--embed") {
					args.push_back("");
					args.erase(args.begin() + i);
					for (int i = args.size() - 1; i >= 0; i--) {
						args[i+1] = args[i];
					}
					args[0] = folder + "/template.o";
				}
			}
			args.push_back("-forigin=0x4000");
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
