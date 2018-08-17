#include "z80cpp_core.hpp"

#include <iostream>
#include <vector>

extern std::vector<uint8_t> int_queue;

z80cpp_core::z80cpp_core() {
	tstates = 0;
}

z80cpp_core::~z80cpp_core() {
	
}

uint64_t z80cpp_core::getCycles() {
	return tstates;
}

uint8_t z80cpp_core::fetchOpcode(uint16_t address) {
	tstates++;
	uint8_t opcode = peek8(address);
	//std::cout << (int)opcode<<"at"<<(int)address<<'\n';
	return opcode;
}

uint8_t z80cpp_core::peek8(uint16_t address) {
	tstates += 3;
	return readRAM(address);
}

void z80cpp_core::poke8(uint16_t address, uint8_t value) {
	tstates += 3;
	writeRAM(address, value);
}

uint16_t z80cpp_core::peek16(uint16_t address) {
	return peek8(address) | (peek8(address+1) << 8);
}

void z80cpp_core::poke16(uint16_t address, uint16_t value) {
	poke8(address, value & 0xFF);
	poke8(address + 1, (value & 0xFF00) >> 8);
}

uint8_t z80cpp_core::inPort(uint16_t port) {
	return in(port);
}

void z80cpp_core::outPort(uint16_t port, uint8_t value) {
	out(port, value);
}

/**
 * Honestly don't know the purpose of this function. Seems to be purely to add
 * extra cycles to the count /shrug
 */
void z80cpp_core::addressOnBus(uint16_t address, int32_t tstates) {
	this->tstates += tstates;
}

void z80cpp_core::interruptHandlingTime(int32_t tstates) {
	this->tstates += tstates;
}

bool z80cpp_core::isActiveINT() {
	if (!int_queue.empty()) {
		return true;
	}
	return false;
}

uint8_t z80cpp_core::interrupt_value() {
	uint8_t v = int_queue.back();
	int_queue.pop_back();
	return v;
}

uint8_t z80cpp_core::breakpoint(uint16_t address, uint8_t opcode) {
	// can be used to modify opcode
	extern Z80 *cpu;
	//std::cout << "HL: "<<cpu->getRegHL() << "\n";
	if (opcode == 0x18 && readRAM(address + 1) == 0xFE) {
		std::cout << "Device: "<<cpu->getRegIY()<<", source: "<<cpu->getRegHL()<<'\n';
		exit(1);
	}
	return opcode;
}

void z80cpp_core::execDone() {
	
}
