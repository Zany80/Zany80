#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <CPU.hpp>

uint16_t PC = 0;

void emulate(uint64_t cycles) {
	for (uint64_t i = 0; i < cycles; i++) {
		cycle();
	}
}

void cycle() {
	uint8_t opcode = readRAM(PC++);
}
