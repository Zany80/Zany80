#define NEEDED_PLUGINS ""
#include <Zany80/Plugins/Generic.hpp>

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

#define CART_COUNT 1
Cartridge *carts[CART_COUNT]{nullptr};

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
		int slot = m.priority;
		if (slot >= 0 && slot <= CART_COUNT - 1) {
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
					// NO MORE VERIFYING! Uses file system now, and verification
					// is the job of the BIOS anyways.
					messageShell("Cart loaded successfully!");
					carts[slot] = c;
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
		for (int i = 0; i < CART_COUNT; i++) {
			if (carts[i] != nullptr) {
				delete carts[i];
				carts[i] = nullptr;
			}
		}
	}
	else if (!strcmp(m.data, "map_disk")) {
		((message_t)(*plugin_manager)["message"])({
			m.priority, "map_bank", (int)strlen("map_bank"), "CPU/z80", (char*)carts[*(uint8_t*)m.context]->raw_data
		}, "Hardware/MMU");
	}
}

extern "C"
uint8_t presentCarts() {
	uint8_t c = 0;
	for (int i = 0; i < CART_COUNT; i++)
		if (carts[i] != nullptr)
			c |= (1 << i);
	return c;
}
