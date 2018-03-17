#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#define NEEDED_PLUGINS ""
#include <Zany80/RAM.hpp>

#include <iostream>

uint8_t * ram = nullptr;

extern "C" {
	uint8_t *getRAM() {
		return ram;
	};
}

void init(liblib::Library *plugin_manager) {
	if (ram != nullptr)
		return;
	ram = new uint8_t[0x10000]{0};
	 //ld bc, **
	//ram[0] = 1;
	 //38519
	 //10010110 01110111
	//ram[1] = 119;
	//ram[2] = 150;
	 //inc bc
	//ram[3] = 3;
	 //ld (bc), a
	//ram[4] = 2;
	 //ld bc, 1
	//ram[5] = 6;
	//ram[6] = 1;
	 //inc b
	//ram[7] = 4;
	 //djnz -3
	//ram[8] = 0x10;
	//ram[9] = -2;
	 //ld a, 0x16
	//ram[10] = 0x3E;
	//ram[11] = 0x16;
	 //ld (15), a
	//ram[12] = 0x32;
	//ram[13] = 0x0F;
	//ram[14] = 0x00;
	 //15 and 16 are used for the `ld d, 0` instruction - 15 is set by the above instruction
	 //inc (hl)
	//ram[17] = 0x34;
}

void write(uint16_t address, uint8_t value) {
	//std::cout << "[RAM] Value "<<(int)value << " written to "<<(int)address<<"\n";
	ram[address] = value;
}

uint8_t read(uint16_t address) {
	//std::cout << "[RAM] Value "<<(int)ram[address] << " read from "<<(int)address<<"\n";
	return ram[address];
}

void polywrite(uint16_t address, uint8_t *value_start, uint16_t length) {
	if ((address + length) < 0x10000) {
		memcpy(ram + address, value_start, length);
	}
	else {
		std::cerr << "[RAM] Data overflow!\n";
	}
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		init((liblib::Library*)m.context);
	}
	else if (!strcmp(m.data, "cleanup")) {
		if (ram!= nullptr){
			delete[] ram;
			ram = nullptr;
		}
	}
}
