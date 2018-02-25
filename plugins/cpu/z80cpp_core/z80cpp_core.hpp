#pragma once

#include <z80cpp/z80operations.h>
#include <z80cpp/z80.h>

class z80cpp_core : public Z80operations {
public:
	z80cpp_core();
	virtual ~z80cpp_core();
	
	uint8_t fetchOpcode(uint16_t address) override;
	uint8_t peek8(uint16_t address) override;
	void poke8(uint16_t address, uint8_t value) override;
	uint16_t peek16(uint16_t address) override;
	void poke16(uint16_t address, uint16_t word) override;
	uint8_t inPort(uint16_t port) override;
	void outPort(uint16_t port, uint8_t value) override;
	void addressOnBus(uint16_t address, int32_t tstates) override;
	void interruptHandlingTime(int32_t tstates) override;
	bool isActiveINT() override;
	uint8_t breakpoint(uint16_t address, uint8_t opcode) override;
	void execDone() override;

	uint64_t getCycles();
private:
	uint64_t tstates;
};
