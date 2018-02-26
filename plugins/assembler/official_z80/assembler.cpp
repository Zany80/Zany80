#include <assembler.hpp>

#include <cstring>
#include <iostream>
#include <fstream>

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
	std::ifstream source_file(source);
	if (!source_file.is_open()) {
		messageShell("Source file doesn't exist!");
		return;
	}
	std::vector<std::string> *input_tokens = new std::vector<std::string>(
		std::istream_iterator<std::string>(source_file),
		std::istream_iterator<std::string>()
	);
	for (std::string &s : *input_tokens) {
		messageShell(((std::string)"Token: " + s).c_str());
	}
	delete input_tokens;
}
