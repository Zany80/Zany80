#include <assembler.hpp>

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

liblib::Library *plugin_manager;

bool isCategory(const char *cat) {
	return (!strcmp(cat, "Assembler")) || (!strcmp(cat, "Development"));
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
		run((std::vector<std::string>*)m.context);
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
		messageShell("Usage: `assemble source target");
	}
}

void assemble(std::string source, std::string target) {
	std::ifstream source_file(source, std::ios::binary);
	if (!source_file.is_open()) {
		messageShell("Source file doesn't exist!");
		return;
	}

	MachineObject object;
	
	std::string line;

	while (std::getline(source_file, line)) {
		object.source_lines.push_back(line);
	}

	source_file.close();
	
	std::string buffer;
	buffer_position p;
	for (int i = 0; i < object.source_lines.size();i++) {
		line = object.source_lines[i];
		object.mapped_tokens.push_back(object.tokens.size());
		
		buffer = "";

		p = normal;
		
		for (char c : line) {
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
							object.tokens.push_back(buffer);
							buffer = "";
						}
					}
					else {
						buffer += c;
					}
					break;
				default:
					if (c == ' ' || c == '\t') {
						p = normal;
						object.tokens.push_back(buffer);
						buffer = "";
					}
					else {
						buffer += c;
					}
			}
		}

		if (buffer != "") {
			object.tokens.push_back(buffer);
		}
		
		messageShell(((std::string)"Line" + std::to_string(i + 1)).c_str());
		for (int j = object.mapped_tokens[i]; j < object.tokens.size();j++) {
			messageShell(((std::string)"Token: " + object.tokens[j]).c_str());
		}
		
	}

	
	
	std::ofstream target_file(target, std::ios::binary | std::ios::trunc);
	if (!target_file.is_open()) {
		messageShell("Error opening target file for writing!");
		return;
	}
	target_file << object;
	target_file.close();

	std::ifstream t(target, std::ios::binary);
	if (t.is_open()) {
		messageShell("Successfully written! Testing output:");
		object.tokens.clear();
		t >> object;
		for (int j = 0; j < object.tokens.size();j++) {
			messageShell(((std::string)"Token: " + object.tokens[j]).c_str());
		}
		t.close();
	}
	
}

std::ostream& operator<<(std::ostream &stream, const MachineObject &object) {
	stream << object.source_lines << object.tokens << object.mapped_tokens << object.instructions;
}

std::ostream& operator<<(std::ostream &stream, const Instruction& i) {
	stream << "Instruction";
}

std::istream& operator>>(std::istream &stream, MachineObject& object) {
	stream >> object.source_lines;
	stream >> object.tokens;
	stream >> object.mapped_tokens;
	stream >> object.instructions;
}

MachineObject::MachineObject() {
	
}
