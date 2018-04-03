#include "z80cpp_core.hpp"

#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8
#define NEEDED_PLUGINS "RAM/16_8"
#define OVERRIDE_RAM
#include <Zany80/CPU.hpp>

#include <Zany80/Drawing.hpp>

#include <iostream>
#include <sstream>
#include <vector>

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

sf::Color palette[] = {
	{0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255},
	{3,224,187},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}
};

std::vector<uint8_t> int_queue;

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		core = new z80cpp_core();
		cpu = new Z80(core);
		for (int i = 0x0000; i < 0xFFFF;i++)
			cpu->setBreakpoint(i, true);
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
		int_queue.clear();
		cpu->reset();
	}
	else if (!strcmp(m.data, "interrupt")) {
		int_queue.push_back(*(uint8_t*)m.context);
	}
	else if (!strcmp(m.data, "unhalt")) {
		cpu->setHalted(false);
	}
}

void cycle() {
	uint64_t pstates = getCycles();
	if (to_execute ++< 1)
		return;
	/**
	 * Executes one *instruction*, *not* one cycle. to_execute keeps track of
	 * how many extra cycles have been executed.
	 */
	cpu->execute();
	to_execute -= (getCycles() - pstates);
}

uint64_t getCycles() {
	return core->getCycles();
}

void out(uint16_t port, uint8_t value) {
	//std::cout << (int)cpu->getRegA()<<' '<< (int)cpu->getRegB()<<' '<< (int)cpu->getRegC()<<' '<< (int)cpu->getRegD()<<' '<< (int)cpu->getRegE()<<' '<< (int)cpu->getRegH()<<' '<< (int)cpu->getRegL()<<"\n";
	if ((port & 0xFF) == 0) {
		if (value == 0) {
			// b, c, e - x, y, color
			std::string string = "";
			for (int i = 0; ;i++) {
				char c = readRAM(i + cpu->getRegHL());
				if (c == 0)
					break;
				string += c;
			}
			text(string, cpu->getRegB(), cpu->getRegC(), palette[cpu->getRegE()]);
		}
		else if (value == 1) {
			clear(palette[cpu->getRegB()]);
		}
	}
	else if ((port & 0xFF) == 1) {
		static int state = 0;
		switch (state) {
		case 0:
			switch (value) {
			case 0:
				state = 1;
				break;
			default:
				std::cerr << "Unrecognized command "<<(int)value<<"!\n";
			}
			break;
		case 1:
			if (value & 0x04) {
				((textMessage_t)(*plugin_manager)["textMessage"])("map_data","CPU/z80;Runner/ROM");
			}
			else {
				uint8_t bank = (value & 0x03);
				uint8_t disk = 0;
				//std::cout << "[CPU/Monitor] Mapping disk to bank "<<(int)bank<<'\n';
				((message_t)(*plugin_manager)["message"])({
					bank, "map_disk", (int)strlen("map_disk"), "CPU/z80", (char *)&disk
				}, "Hardware/CartridgeManager");
				state = 0;
			}
			break;
		}
	}
	else {
		std::cout << (int)value << " written to "<<(int)(port&0xFF) <<"\n";
	}
}

uint8_t in(uint16_t port) {
	if ((port & 0xFF) == 0x00) {
		uint8_t val = 0;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
			val |= 1;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
			val |= 2;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			val |= 4;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
			val |= 8;
		}
		return val;
	}
	else if ((port & 0xFF) == 0x01) {
		return ((uint8_t(*)())(*((liblib::Library*(*)(const char *))(*plugin_manager)["getPlugin"])("Hardware/CartridgeManager"))["presentCarts"])();
	}
}
