#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#define NEEDED_PLUGINS ""
#include <Zany80/RAM.hpp>

#include <iostream>

liblib::Library *plugin_manager;

bool isSignatureCompatible(const char *sig) {
	return !strcmp(s(sig(ADDRESS_BUS_SIZE,DATA_BUS_SIZE)), sig) ||
		!strcmp(sig, "MMU");
}

bool isCategory(const char *sig) {
	return !strcmp(sig, "Hardware");
}

bool isType(const char *sig) {
	return !strcmp(sig, "MMU");
}

uint8_t *banks[4]{nullptr};

uint8_t *stack = nullptr;

void write(uint16_t address, uint8_t value) {
	banks[address / 0x4000][address % 0x4000] = value;
}

uint8_t read(uint16_t address) {
	uint8_t v = banks[address / 0x4000][address % 0x4000];
	//std::cout << "Value: "<<(int)v<<'\n';
	return v;
}

void polywrite(uint16_t address, uint8_t *value_start, uint16_t length) {
	if (address + length < address) {
		((message_t)(*plugin_manager)["message"])({
			0, "history", (int)strlen("history"), "Hardware/MMU", "[MMU] Invalid write attempt!"
		}, "Runner/Shell");
		((void(*)(RunnerType,const char *))(*plugin_manager)["activateRunner"])(Shell,"");
		return;
	}
	for (uint16_t i = address; i < address + length; i++) {
		write(i, value_start[i]);
	}
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		plugin_manager = (liblib::Library*)m.context;
		stack = new uint8_t[0x4000];
		banks[3] = stack;
	}
	else if (!strcmp(m.data, "cleanup")) {
		
	}
	else if (!strcmp(m.data, "map_bank")) {
		if (m.priority < 0 || m.priority > 3) {
			((message_t)(*plugin_manager)["message"])({
				0, "history", (int)strlen("history"), "Hardware/MMU",
					"[MMU] Invalid bank mapping attempt!\n"
			}, "Runner/Shell");
			((void(*)(RunnerType,const char *))(*plugin_manager)["activateRunner"])(Shell,"");
		}
		std::cout << "[MMU] mapping bank "<<m.priority<<"\n";
		banks[m.priority] = (uint8_t*)m.context;
	}
}
