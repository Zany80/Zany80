#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#ifdef ORYOL_WINDOWS
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

#include "simple_shell.h"
#include <shell_config.h>

#include <IO/IO.h>
#include <IMUI/IMUI.h>

#include <Zany80/API.h>

bool SimpleShell::supports(String type) {
	return Array<String>({"Perpetual", "Shell"}).FindIndexLinear(type) != InvalidIndex;
}

SimpleShell::SimpleShell() {
	instance = plugin_instances++;
	strcpy(command_string, "");
	commands = {
		{"exit", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				//TODO:IMPLEMENT
				shell->output("Error: not yet implemented.");
				return false;
			},
			.help = "Exits the shell.",
			.detailed_help = "Exits the shell. The most recent application will be displayed; if none are open, this returns to the menu."
		}},
		{"echo", {
			.function = [](Array<String> args, SimpleShell *shell) -> int{
				StringBuilder all_args;
				for (String s: args) {
					all_args.Append(s);
					all_args.Append(' ');
				}
				if (all_args.Length())
					all_args.PopBack();
				shell->output(all_args.GetString());
				return true;
			},
			.help = "Echoes all received arguments to the history buffer.\n",
			.detailed_help = "Echoes all received arguments to the history buffer.\n"
							 "Run `echo Hello there, self!` to see it in action."
		}},
		
		{"load", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				//TODO: implement
				shell->output("Error: not yet implemented.");
				return false;
			},
			.help = "Loads a disc",
			.detailed_help =
			"Loads a disc into the tray."
		}},
	
		{"run", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				//TODO: implement
				shell->output("Error: not yet implemented.");
				return false;
			},
			.help = "Launches the specified cart."
		}},

		{"reset", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				//TODO: implement
				shell->output("Error: not yet implemented.");
				return false;
			},
			.help = "Reset the CPU",
			.detailed_help = "Resets the CPU. This restarts any game that is open."
		}},

		{"cd", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				if (args.Size() != 1) {
					shell->output("Usage: `cd directory`");
					return true;
				}
				#if ORYOL_WINDOWS
				if (!chdir(args[0].c_str())) {
					StringBuilder s;
					s.AppendFormat("SetCurrentDirectory failed! #%d";GetLastError());
					shell->output(s.GetString());
					return false;
				}
				#elif ORYOL_POSIX
				if (chdir(args[0].AsCStr()) == -1) {
					switch (errno) {
						case ENOENT:
							shell->output("No such directory: %s", args[0].AsCStr());
							break;
						default:
							shell->output("Error!");
					}
					return false;
				}
				#else
				shell->output("Error: cd only supported on Windows and on POSIX-compatible platforms!");
				return false;
				#endif
				shell->updateWorkingDirectory();
				return true;
			},
			.help = "Changes the current directory"
		}},

		{"assembler", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				Array<Plugin*> tools = getPlugins("Toolchain");
				if (tools.Size() == 0) {
					shell->output("Error: no assembler plugins loaded!");
					return 1;
				}
				Array<String> inputs, libs;
				String target = "";
				for (int i = 0; i < args.Size(); i++) {
					String arg = args[i];
					if (arg.Length() && arg.Front() == '-') {
						if (arg == "-c") {
							libs.Add(processFileURI("lib:libc.o"));
						}
						else if (arg == "-o") {
							if (i + 1 == args.Size()) {
								shell->output("-o specified without output file!");
								return 9;
							}
							else {
								if (target != "") {
									shell->output("Overriding previous -o of %s", target.AsCStr());
								}
								target = args[++i];
							}
						}
						else {
							shell->output("Unknown option: %s", arg.AsCStr());
						}
					}
					else {
						inputs.Add(arg);
					}
				}
				if (inputs.Size() < 1) {
					shell->output("Error: must provide input and output files!");
					return 7;
				}
				ToolchainPlugin *t = nullptr;
				Array<ToolchainPlugin*> assemblers;
				for (Plugin *p : tools) {
					t = dynamic_cast<ToolchainPlugin*>(p);
					if (t->supportedTransforms().Contains(".asm"))
						assemblers.Add(t);
				}
				if (assemblers.Size() == 0) {
					shell->output("Error: no assembler plugins loaded!");
					return 2;
				}
				t = nullptr;
				for (ToolchainPlugin *a : assemblers) {
					if (a->getChain() == "Official") {
						t = a;
						break;
					}
				}
				if (t == nullptr) {
					t = assemblers[0];
					shell->output("Warning: official assembler not found. Resorting to %s", t->getChain().AsCStr());
				}
				t->setVerbosity((int)Log::GetLogLevel());
				StringBuilder output;
				if (target == "") {
					target = inputs[inputs.Size() - 1];
					inputs.PopBack();
				}
				// Make sure target isn't a library
				if (target.Length() > processFileURI("lib:").Length()) {
					if (!strncmp(target.AsCStr(), processFileURI("lib:").AsCStr(), processFileURI("lib:").Length())) {
						shell->output("Target is a library: %s", target.AsCStr());
						return 8;
					}
				}
				// TODO: update
				// Add libraries to BEGINNING of inputs (temporary to ensure boot code in proper location)
				for (String s : libs) {
					inputs.Insert(0, s);
				}
				int code = t->transform(inputs, target, &output);
				Array<String> newMessages;
				output.Tokenize("\n", newMessages);
				for (String s : newMessages) {
					shell->output(s);
				}
				return code;
			},
			.help = "Invokes the assembler to turn an assembly input file into an object file."
		}},
		
		{"compile", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				args.Add("-c");
				args.Add("-I");
				args.Add("libc:include");
				args.Add("-Wp,-include -Wp,system.h");
				args.Add("-Wp,-include -Wp,output.h");
				args.Add("-Wp,-include -Wp,input.h");
				//TODO: implement
				shell->output("Error: not yet implemented.");
				return false;
			},
			.help = "Invokes the C compiler."
		}},
	
		{"link", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				//~ args.Add("-forigin=0x4000");
				// TODO: Implement
				shell->output("Not yet implemented");
				return false;
			},
			.help = "Invokes the linker to turn object files into a usable ROM"
		}},
	
		{"clear", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				shell->history.Clear();
				return true;
			},
			.help = "Clears the output"
		}},
	
		{"help", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				if (args.Size() == 0) {
					shell->output("Commands: ");
					for (auto pair : shell->commands) {
						shell->output("\t%s: %s",pair.key.AsCStr(),pair.value.help.AsCStr());
					}
				}
				else if (args.Size() == 1) {
					if (shell->commands.Contains(args[0])) {
						String help = shell->commands[args[0]].detailed_help;
						if (help == "") {
							shell->output("No help available for that command!");
						}
						else {
							shell->output(help);
						}
					}
					else {
						shell->output("No such command!");
					}
				}
				else {
					shell->output("Usage: `help [command]`");
				}
				return true;
			},
			.help = "Prints out information on commands. For more information, run `help help`",
			.detailed_help = "Prints out information on commands. With no arguments, gives general help. "
			"If run with the name of a command (e.g. `help run`), gives more detailed information."
		}},

		{"ls", {
			.function = [](Array<String> args, SimpleShell *shell) -> int {
				if (!args.Size()) {
					args.Add(".");
				}
				for (String path : args) {
					StringBuilder s;
					s.AppendFormat(256, "%s: ", path.AsCStr());
					shell->output(s.GetString());
					s.Set("\t");
					for (String _s : readDirectory(path.AsCStr())) {
						s.AppendFormat(256, "%s ", _s.AsCStr());
					}
					shell->output(s.GetString());
				}
				return true;
			},
			.help = "Prints out a list of files in the current folder.",
			.detailed_help = "Prints out a list of files in the current folder."
		}},
	};

	autocompletion_map = {
		{"Help", [](String last_word, SimpleShell *shell) -> String {
			Array<String> possible_commands;	
			StringBuilder s;
			for (auto pair : shell->commands) {
				s.Set(pair.key);
				s.Append(' ');
				// Only continue if word being autocompleted is shorter than this command
				if (last_word.Length() >= s.Length())
					continue;
				if (s.GetSubString(0, last_word.Length()) == last_word)
					possible_commands.Add(s.GetString());
			}
			for (int i = 0; i < possible_commands.Size(); i++) {
				StringBuilder s = possible_commands[i];
				s.Append(" ");
				possible_commands[i] = s.GetString();
			}
			if (possible_commands.Size() == 1) {
				return possible_commands[0];
			}
			if (possible_commands.Size()) {
				StringBuilder c;
				for (String s : possible_commands) {
					c.Append(s);
				}
				c.PopBack();
				shell->output(c.GetString());
			}
			return last_word;
		}}
	};
	
	this->output("Zany80 Simple Shell...");
	this->output("Version " PROJECT_VERSION);
	this->output("Hello! If you need assistance, contact me at pleasantatk@gmail.com");
	this->output("Also, for help, you can use `help`. It's an extremely helpful command ;)");
	this->working_directory = "";
	this->updateWorkingDirectory();
}

SimpleShell::~SimpleShell() {
	
}

void SimpleShell::frame(float delta) {
	ImGui::SetNextWindowSizeConstraints(ImVec2(530,200), ImVec2(FLT_MAX, FLT_MAX));
	char buf[256];
	sprintf(buf, "SimpleShell##%lu",instance);
	ImGui::Begin(buf);
	ImGui::Text("SimpleShell");
	for (int i = 0; i < history.Size(); i++) {
		ImGui::TextWrapped("%s", history[i].AsCStr());
	}
	if (ImGui::InputText("", command_string, 255,ImGuiInputTextFlags_NoUndoRedo|ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, [](ImGuiTextEditCallbackData *data) -> int {
		switch (data->EventFlag) {
			case ImGuiInputTextFlags_CallbackCompletion:
				StringBuilder command_list;
				for (auto pair : ((SimpleShell*)data->UserData)->commands) {
					command_list.Append(pair.key);
					command_list.Append(' ');
				}
				if (command_list.Length())
					command_list.PopBack();
				StringBuilder current_command;
				current_command.Append(data->Buf);
				// Strip beginning whitespace
				// TODO: Fixup
				while (isspace(current_command.AsCStr()[0])) {
					if (current_command.Length() == 1) {
						((SimpleShell*)data->UserData)->output(command_list.GetString());
						return 0;
					}
					current_command.Set(current_command.GetSubString(1, EndOfString));
				}
				// Find end of first word
				int i;
				for (i = 0; i < current_command.Length() && !isspace(current_command.AsCStr()[i]); i++);
				if (i == 0) {
					// String is empty
					((SimpleShell*)data->UserData)->output(command_list.GetString());
					return 0;
				}
				String first_word = current_command.GetSubString(0, i);
				if (i == current_command.Length()) {
					// Only one word, treat it as a command
					Array<String> possible_commands;
					for (auto pair : ((SimpleShell*)data->UserData)->commands) {
						StringBuilder s = pair.key;
						if (s.Length() > first_word.Length() && s.GetSubString(0, first_word.Length()) == first_word)
							possible_commands.Add(pair.key);
					}
					for (int i = 0; i <possible_commands.Size(); i++) {
						StringBuilder s = possible_commands[i];
						s.Append(" ");
						possible_commands[i] = s.GetString();
					}
					if (possible_commands.Size() == 1) {
						data->DeleteChars(0, current_command.Length());
						data->InsertChars(0, possible_commands[0].AsCStr());
					}
					else if (possible_commands.Size()) {
						StringBuilder c;
						for (String s : possible_commands) {
							c.Append(s);
						}
						c.PopBack();
						((SimpleShell*)data->UserData)->output(c.GetString());
					}
				}
				else {
					// Get last argument
					StringBuilder last_word = "";
					for (i = current_command.Length(); i > 0 &&!isspace(current_command.AsCStr()[i]); i--);
					if (i + 1 != current_command.Length())
						last_word.Set(current_command.GetSubString(i + 1, EndOfString));
					String(*autocompleter)(String, SimpleShell*) = [](String word, SimpleShell *shell) -> String {
						Array<String> files = readDirectory(".");
						bool changed = true;
						while (changed) {
							changed = false;
							for (int i = 0; i < files.Size(); i++) {
								StringBuilder file = files[i];
								if (word.Length()
							&& (file.Length() <= word.Length()
							|| file.GetSubString(0, word.Length()) != word)) {
									changed = true;
									files.Erase(i);
									break;
								}
							}
						}
						if (files.Size() == 1) {
							StringBuilder f = files[0];
							f.Append(' ');
							return f.GetString();
						}
						else if (files.Size()) {
							StringBuilder s;
							for (String f : files) {
								s.Append(f);
								s.Append(' ');
							}
							s.PopBack();
							shell->output(s.GetString());
						}
						return word;
					};
					if (((SimpleShell*)data->UserData)->autocompletion_map.Contains(first_word)) {
						autocompleter = ((SimpleShell*)data->UserData)->autocompletion_map[first_word];
					}
					// autocomplete last word
					StringBuilder s = data->Buf;
					for (i = s.Length(); i > 0 && !isspace(data->Buf[i]); i--);
					// i contains address of last space character
					data->DeleteChars(i, current_command.Length() - i);
					data->InsertChars(i, autocompleter(last_word.GetString(), ((SimpleShell*)data->UserData)).AsCStr());
					data->InsertChars(i, " ");
				}
				break;
		}
		return 0;
	}, this)) {
		last_result = this->processCommand();
	}
	if (outputted) {
		outputted--;
		if (scroll_on_output)
			ImGui::SetScrollHereY(1);
	}
	ImGui::End();
}

int SimpleShell::processCommand() {
	StringBuilder c;
	c.Append(command_string);
	// Strip beginning whitespace
	while (isspace(c.AsCStr()[0])) {
		if (c.Length() == 1) {
			output("Can't execute whitespace!");
			return -3;
		}
		c.Set(c.GetSubString(1, EndOfString));
	}
	// Strip trailing whitespace
	while (isspace(c.AsCStr()[c.Length() - 1])) {
		c.Set(c.GetSubString(0, c.Length() - 1));
	}
	strcpy(command_string, c.GetString().AsCStr());
	int space = c.FindFirstOf(0, EndOfString, " ");
	Array<String> args;
	StringBuilder command;
	command.Append(command_string);
	if (space != InvalidIndex && space != 0) {
		command.Set(command.GetSubString(0, space));
		c.Set(c.GetSubString(space+1,EndOfString));
		while ((space = c.FindFirstOf(0, EndOfString, " ")) != InvalidIndex) {
			if (space != 0) {
				args.Add(c.GetSubString(0,space));
			}
			c.Set(c.GetSubString(space+1, EndOfString));
		}
		if (c.Length() > 0) {
			args.Add(c.GetString());
		}
	}
	output("%s$ %s", working_directory.AsCStr(), command_string);
	strcpy(command_string, "");
	if (commands.Contains(command.GetString())) {
		shell_command _command = commands[command.GetString()];
		return _command.function(args, this);
	}
	else {
		StringBuilder message = "No such command: \"";
		message.Append(command.GetString());
		message.Append("\"");
		output(message.GetString());
		return -1;
	}
}

void SimpleShell::updateWorkingDirectory() {
	char _working_directory[FILENAME_MAX];
	if (GetCurrentDir(_working_directory, FILENAME_MAX)) {
		working_directory = _working_directory;
	}
	else {
		working_directory = "";
	}
}

void SimpleShell::output(String s) {
	output("%s", s.AsCStr());
}
	
void SimpleShell::output(const char *fmt, ...) {
	constexpr int buf_size = 256;
	char buf[buf_size];
	va_list args;
    va_start(args, fmt);
    vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    history.Add(buf);
    outputted += 2;
}

Array<String> SimpleShell::getCommands() {
	Array<String> c;
	for (KeyValuePair<String, shell_command> command : commands) {
		c.Add(command.key);
	}
	return c;
}

int SimpleShell::execute(String command) {
	size_t size = command.Length() + 1;
	char buf[size];
	memcpy(buf, command_string, size);
	strcpy(command_string, command.AsCStr());
	int result = processCommand();
	memcpy(command_string, buf, size);
	return result;
}

#ifndef ORYOL_EMSCRIPTEN
extern "C" Plugin *make() {
	return new SimpleShell;
}

extern "C" const char *getName() {
	return "Simple Shell";
}
#endif
