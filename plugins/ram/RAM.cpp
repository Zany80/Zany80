#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <Zany80/RAM.hpp>

#include <iostream>

uint8_t * ram = nullptr;

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *plugin_manager) {
	if (ram != nullptr)
		return;
	ram = new uint8_t[0x10000]{0};
	// ld bc, **
	ram[0] = 1;
	// 38519
	// 10010110 01110111
	ram[1] = 119;
	ram[2] = 150;
	// inc bc
	ram[3] = 3;
	// ld (bc), a
	ram[4] = 2;
}

void cleanup() {
	if (ram!= nullptr){
		delete[] ram;
		ram = nullptr;
	}
}

void write(uint16_t address, uint8_t value) {
	std::cout << "[RAM] Value "<<(int)value << " written to "<<(int)address<<"\n";
	ram[address] = value;
}

uint8_t read(uint16_t address) {
	//std::cout << "[RAM] Value "<<(int)ram[address] << " read from "<<(int)address<<"\n";
	return ram[address];
}
