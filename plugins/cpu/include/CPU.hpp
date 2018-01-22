#pragma once
#include <Plugins.hpp>
#include <liblib/liblib.hpp>

#define _t(x) uint ## x ## _t
#define t(x) _t(x)

#ifdef ADDRESS_BUS_SIZE
#define ADDRESS_BUS_SIZE_T t(ADDRESS_BUS_SIZE)
#else
#error ADDRESS_BUS_SIZE must be defined!
#endif

#ifdef DATA_BUS_SIZE
#define DATA_BUS_SIZE_T t(DATA_BUS_SIZE)
#else
#error DATA_BUS_SIZE must be defined!
#endif

typedef void(*write_t)(ADDRESS_BUS_SIZE_T,DATA_BUS_SIZE_T);
typedef DATA_BUS_SIZE_T(*read_t)(ADDRESS_BUS_SIZE_T);

write_t writeRAM;
read_t readRAM;

extern "C" {
	PluginType *getType();
	HardwareType *getHardwareType();
	void emulate(uint64_t cycles);
	void cycle();
	void setRAM(liblib::Library *RAM);
	uint8_t getAddressBusSize();
	uint8_t getDataBusSize();
}

PluginType type = Hardware;
HardwareType hw_type = CPU;

PluginType *getType(){
	return &type;
}

HardwareType *getHardwareType() {
	return &hw_type;
}

void setRAM(liblib::Library *RAM) {
	// Deliberately *don't* handle the exception - let the main program take care of it
	// Caches the functions instead of fetching them every time for performance reasons
	writeRAM = (write_t)(*RAM)["write"];
	readRAM = (read_t)(*RAM)["read"];
}
uint8_t getAddressBusSize() {
	return ADDRESS_BUS_SIZE;
}
uint8_t getDataBusSize() {
	return DATA_BUS_SIZE;
}

#ifndef OVERRIDE_EMULATE

void emulate(uint64_t cycles) {
	for (uint64_t i = 0; i < cycles; i++) {
		cycle();
	}
}

#endif
