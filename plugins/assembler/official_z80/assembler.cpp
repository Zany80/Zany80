#include <assembler.hpp>
#include <MachineObject.hpp>

#define NEEDED_PLUGINS ""

#include <Zany80/Plugins/Generic.hpp>
#include <Zany80/GenericException.hpp>

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

std::string workingDirectory() {
	char *working_directory = new char[256];
	if (!GetCurrentDir(working_directory, 256)) {
		strcpy(working_directory, ".");
	}
	std::string s = working_directory;
	delete[] working_directory;
	return s;
}

liblib::Library *plugin_manager;

bool isCategory(const char *cat) {
	return (!strcmp(cat, "Assembler")) || (!strcmp(cat, "Development") || (!strcmp(cat, "Linker")));
}

bool isType(const char *type) {
	return (!strcmp(type, "z80"));
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data,"init")) {
		plugin_manager = (liblib::Library*)m.context;
	}
	else if (!strcmp(m.data,"cleanup")) {
		
	}
	else if (!strcmp(m.data, "invoke")) {
		if (m.priority == 0) {
			// assemble
			run((std::vector<std::string>*)m.context);
		}
		else {
			// link
			link((std::vector<std::string>*)m.context);
		}
	}
}

void messageShell(const char *message) {
	((message_t)(*plugin_manager)["message"])({
		0, "history", (int)strlen("history"), "Assembler/z80", message
	}, "Runner/Shell");
}

void run(std::vector<std::string> *args) {
	if (args->size() == 2) {
		assemble((*args)[0], (*args)[1]);
	}
	else {
		messageShell("Usage: `assemble source target`");
	}
}

void link(std::vector<std::string> *args) {
	
}

void tokenizeLine(std::string line, std::vector<std::string> &tokens) {
	std::string buffer = "";
	buffer_position p = normal;
	
	for (int i = 0; i < line.size(); i++) {
		char c = line[i];
		switch (p) {
			case normal:
				if (c == '"') {
					buffer += c;
					p = string;
				}
				else if (c == '\'') {
					buffer += c;
					p = character;
				}
				else if (c == ' ' || c == '\t') {
					// end of the token
					if (buffer != "") {
						tokens.push_back(buffer);
						buffer = "";
					}
				}
				else if (c == ',' && (i + 1 == line.size() || line[i + 1] == ' ' || line[i + 1] == '\t')) {
					// suppress the comma
				}
				else {
					buffer += c;
				}
				break;
			default:
				if (c == ' ' || c == '\t') {
					p = normal;
					tokens.push_back(buffer);
					buffer = "";
				}
				else {
					buffer += c;
				}
		}
	}

	if (buffer != "") {
		tokens.push_back(buffer);
	}

}

void assemble(std::string source, std::string target) {
	std::ifstream source_file(source, std::ios::binary);
	if (!source_file.is_open()) {
		messageShell("Source file doesn't exist!");
		return;
	}

	MachineObject o;
	
	std::string line;
	int i = 0;
	bool error = false;
	std::string file = workingDirectory() + "/" + source;
	while (std::getline(source_file, line)) {
		i++;
		std::vector<std::string> tokens;
		tokenizeLine(line, tokens);
		for (int j = 0; j < tokens.size(); j++) {
			std::string token = tokens[j];
			if (token == ".org") {
				if (j + 1 == tokens.size()) {
					error = true;
					messageShell(".org requires argument!");
				}
				else {
					Argument a1, a2;
					a1.m = SET_PC;
					try {
						a2.i = parseNumber(tokens[++j]);
					}
					catch(GenericException &e) {
						messageShell(e.what());
						error = true;
						continue;
					}
					Instruction instruction(file, i, META, a1, a2);
					o.addInstruction(instruction);
				}
			}
			//else if (token == "ld") {
				
			//}
			else if (token == "nop") {
				o.addInstruction(Instruction(file, i, NOP));
			}
			else {
				messageShell(("Unrecognized token: " + token).c_str());
			}
		}
	}
	source_file.close();
	if (error) {
		messageShell("Failed to assemble!");
		return;
	}

	std::ofstream target_file(target);
	if (target_file.is_open()) {
		target_file << o;
		target_file.close();
		std::ifstream stream(target);
		if (stream.is_open()) {
			MachineObject a;
			stream >> a;
			for (Instruction &i : a.instructions) {
				std::stringstream s;
				s << i;
				messageShell(s.str().c_str());
			}
			stream.close();
		}
	}
	else {
		messageShell("Error writing assembled instructions!");
	}
	
	//Instruction i(LDR8R8, r8_arg(A), r8_arg(B));
	//std::ofstream stream(target);
	//if (stream.is_open()) {
		//stream << i;
		//stream.close();
		//std::ifstream t(target);
		//if (t.is_open()) {
			//Instruction v;
			//t >> v;
			//std::stringstream s;
			//s << v;
			//messageShell(s.str().c_str());
		//}
		//else {
			//messageShell("Error reading back!");
		//}
	//}
	//else {
		//messageShell("Error writing!");
	//}
	
}

std::istream &operator>>(std::istream &stream, std::vector<std::string> &v) {
	int size;
	stream >> size;
	//std::cout << size;
	v.resize(size);
	std::string buffer;
	// discard remainder of first line - simpler than specializing std::ostream
	// std::vector<std::string>.
	std::getline(stream, buffer);
	for (int i = 0; i < size; i++) {
		std::getline(stream, buffer);
		v[i] = buffer;
	}
}

int parseNumber(std::string &s) {
	int value = 0;
	std::string hex_values="0123456789ABCDEF";
	if (s.find_first_of("0x") == 0) {
		int8_t places = s.size() - 2;
		if (places > 0) {
			for (int i = 2; i < s.size(); i++) {
				char c = toupper(s[i]);
				if (!isxdigit(c)) {
					throw GenericException("Invalid hex string!");
				}
				value += hex_values.find(c) * pow(16, places - (i - 1));
			}
		}
		else {
			throw GenericException("Invalid hex string!");
		}
	}
	else {
		// if no special form is identified, assume decimal
		for (int i = 0; i < s.size(); i++) {
			char c = s[i];
			if (!isdigit(c)) {
				throw GenericException("Invalid decimal string!");
			}
			value += hex_values.find(c) * pow(10, s.size() - i - 1);
		}
	}
	return value;
}
