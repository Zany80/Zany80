#define NEEDED_PLUGINS ""
#include <Zany80/Generic.hpp>

#include "Cartridge.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

bool isCategory(const char *sig) {
	return !strcmp(sig, "Hardware") || !strcmp(sig, "Cartridge");
}

bool isType(const char *sig) {
	return !strcmp(sig, "Cartridges") || !strcmp(sig, "CartridgeManager") ||
		(strlen(sig) == 1 && sig[0] >= '0' && sig[0] <= '3');
}

liblib::Library *plugin_manager;

Cartridge *carts[4]{nullptr};

void messageShell(std::string message) {
	((message_t)(*plugin_manager)["message"])({
		0,"history",(int)strlen("history"),"Hardware/Cartridges",("[CartridgeManager] " + message).c_str()
	},"Runner/Shell");
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		plugin_manager = (liblib::Library*)m.context;
	}
	else if (!strcmp(m.data, "load_cart")) {
		messageShell("Test");
		int slot = m.priority;
		if (slot >= 0 && slot <= 3) {
			std::ifstream file(m.context, std::ios::ate | std::ios::binary);
			if (file.is_open()) {
				Cartridge *c = new Cartridge;
				memset(c->raw_data, 0, 16 * 0x4000);
				int size = file.tellg();
				if (size > sizeof(Cartridge)) {
					size = sizeof(Cartridge);
				}
				file.seekg(0);
				if (size >= sizeof(c->metadata)) {
					file.read((char*)c->raw_data, size);
					// Cart loaded, verify it
					// Magic value is equivalent to ASCII "ZANY"
					if (c->metadata.zany == 0x594E415A && c->metadata.start < size
						&& c->metadata.title < size) {
						carts[slot] = c;
						messageShell("Cart loaded successfully!");
					}
					else {
						messageShell("Invalid ROM!");
					}
				}
				else {
					messageShell("Invalid ROM!");
				}
				file.close();
			}
			else {
				messageShell("Error loading cart!");
			}
		}
		else {
			messageShell("Slot "+std::to_string(m.priority) + " doesn't exist!");
		}
	}
	else if (!strcmp(m.data, "cleanup")) {
		for (int i = 0; i < 4; i++) {
			if (carts[i] != nullptr) {
				delete carts[i];
				carts[i] = nullptr;
			}
		}
	}
}

extern "C"
uint8_t presentCarts() {
	uint8_t c = 0;
	for (int i = 0; i < 4; i++)
		if (carts[i] != nullptr)
			c |= (1 << i);
	return c;
}
