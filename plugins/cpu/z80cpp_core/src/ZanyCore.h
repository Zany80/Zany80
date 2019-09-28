#pragma once

#include <Zany80/Plugin.h>
#include <z80operations.h>
#include <z80.h>

#define Hz * 1
#define KHz * 1000 Hz
#define MHz * 1000 KHz

class ZanyCore : public Z80operations {
public:
	ZanyCore();
	virtual ~ZanyCore();
	
	/* Read opcode from RAM */
	virtual uint8_t fetchOpcode(uint16_t address);

	/* Read/Write byte from/to RAM */
	virtual uint8_t peek8(uint16_t address);
	virtual void poke8(uint16_t address, uint8_t value);

	/* Read/Write word from/to RAM */
	virtual uint16_t peek16(uint16_t adddress);
	virtual void poke16(uint16_t address, uint16_t word);

	/* In/Out byte from/to IO Bus */
	virtual uint8_t inPort(uint16_t port);
	virtual void outPort(uint16_t port, uint8_t value);

	/* Put an address on bus lasting 'wstates' cycles */
	virtual void addressOnBus(uint16_t address, int32_t wstates);

	/* Clocks needed for processing INT and NMI */
	virtual void interruptHandlingTime(uint8_t wstates);

	/* Callback to know when the INT signal is active */
	virtual bool isActiveINT(void);
	
	/* Callback for notify at PC address */
	virtual uint8_t breakpoint(uint16_t address, uint8_t opcode);

	/* Callback to notify that one instruction has ended */
	virtual void execDone(void);
	
	list_t *get_registers();
	uint64_t executedCycles();
	void execute(uint32_t cycles);
	void attachToPort(uint32_t port, read_handler_t);
	void attachToPort(uint32_t port, write_handler_t);
	void fireInterrupt(uint8_t IRQ);
	bool loadROM(const char *path);
	void reset();
	
private:
	// Item type: uint8_t
	list_t *interrupt_buffer;
	Z80 cpu;
	unsigned long tstates;
	read_handler_t inport[256];
	write_handler_t outport[256];
};
