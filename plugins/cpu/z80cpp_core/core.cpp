#include "z80cpp_core.hpp"

#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8
#define NEEDED_PLUGINS "RAM/16_8"
#define OVERRIDE_RAM
#include <Zany80/CPU.hpp>

#include <iostream>

const char *signature = "z80";

z80cpp_core *core = nullptr;
Z80 *cpu = nullptr;

int8_t to_execute;

liblib::Library *plugin_manager, *_RAM;

void setRAM(liblib::Library *RAM) {
	_RAM = RAM;
	readRAM = (read_t)(*RAM)["read"];
	writeRAM = (write_t)(*RAM)["write"];
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		core = new z80cpp_core();
		cpu = new Z80(core);
		to_execute = 0;
		plugin_manager = (liblib::Library*)m.context;
	}
	else if (!strcmp(m.data, "cleanup")) {
		if (core != nullptr) {
			delete core;
			delete cpu;
			core = nullptr;
			cpu = nullptr;
		}
	}
	else if (!strcmp(m.data, "reset")) {
		try {
			((textMessage_t)(*plugin_manager)["textMessage"])("boot_flash","CPU/z80;Runner/ROM");
		}
		catch (std::exception){}
		cpu->reset();
	}
	else if (!strcmp(m.data, "setPC")) {
		uint16_t target = *(uint16_t*)m.context;
		uint8_t buffer[3] = {};
		for (int i = 0; i < 3;i++)
			buffer[i] = readRAM(i);
		writeRAM(0, 0xC3);
		writeRAM(1, target & 0xFF);
		writeRAM(2, (target & 0xFF00) >> 8);
		cpu->execute();
		for (int i = 0; i < 3;i++)
			writeRAM(i, buffer[i]);
	}
}

void cycle() {
	uint64_t pstates = getCycles();
	if (to_execute ++< 1)
		return;
	/**
	 * Executes one *instruction*, *not* one cycle. to_execute ke
	 * eps track of
	 * how many extra cycles have been executed.
	 */
	cpu->execute();
	to_execute -= (getCycles() - pstates);
}

uint64_t getCycles() {
	return core->getCycles();
}

void out(uint16_t port, uint8_t value) {
	
}

uint8_t in(uint16_t port) {
	
}
