#include <assembler.hpp>

#include <cstring>
#include <iostream>

std::vector<std::string> *tokens;

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

void run(std::vector<std::string> *args) {
	if (args->size() == 0) {
		((message_t)(*plugin_manager)["message"])({
			0, "history", (int)strlen("history"), "Assembler/z80", "No arguments provided!"
		}, "Runner/Shell");
	}
}
