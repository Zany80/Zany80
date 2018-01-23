#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <Zany80/RAM.hpp>

uint8_t * ram = nullptr;

const char *neededPlugins(){
	return "";
}

void init(liblib::Library *plugin_manager) {
	if (ram != nullptr)
		return;
	ram = new uint8_t[0x10000]{0};
}

void write(uint16_t address, uint8_t value) {
	ram[address] = value;
}

uint8_t read(uint16_t address) {
	return ram[address];
}
