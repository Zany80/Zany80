#define NEEDED_PLUGINS ""
#include <Zany80/Generic.hpp>
#include <Zany80/Zany80.hpp>

#include <cstring>
#include <fstream>

liblib::Library *plugin_manager;

extern "C" {

	bool isCategory(const char *cat) {
		return (!strcmp(cat, "Assembler")) || (!strcmp(cat, "Development") || (!strcmp(cat, "Linker")));
	}

	bool isType(const char *type) {
		return (!strcmp(type, "z80"));
	}
	
}

void messageShell(const char *message) {
	((message_t)(*plugin_manager)["message"])({
		0, "history", (int)strlen("history"), "Assembler/z80", message
	}, "Runner/Shell");
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data,"init")) {
		plugin_manager = (liblib::Library*)m.context;
	}
	else if (!strcmp(m.data,"cleanup")) {
		
	}
	else if (!strcmp(m.data, "invoke")) {
		std::string command = true_folder + "/plugins/binaries/scas ";
		if (m.priority) {
			command += true_folder + "/libc/libc.o ";
		}
		for (std::string s : *((std::vector<std::string>*)m.context)) {
			command += s + " ";
		}
		command += "2>scas_error.log";
		if (system(command.c_str()) == 0) {
			messageShell("Assembled successfully!");
		}
		else {
			std::ifstream error("scas_error.log");
			if (error.is_open()) {
				std::string buf;
				while (std::getline(error, buf)) {
					messageShell(buf.c_str());
				}
				error.close();
				remove("scas_error.log");
			}
			else {
				messageShell("Error!");
			}
		}
	}
}