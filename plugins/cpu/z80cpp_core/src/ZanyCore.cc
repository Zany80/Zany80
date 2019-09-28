#include "ZanyCore.h"
#include "MMU.h"

#include <Zany80/API.h>
#include <Zany80/Zany80.h>

#include <thread>
#include <chrono>
#include <stdlib.h>
#include <string.h>

ZanyCore::ZanyCore() : cpu(this) {
	// TODO: use MakeID.h for a plugin ID generator
	//~ this->instance = plugin_instances++;
	this->tstates = 0;
	memset(inport, 0, sizeof(read_handler_t) * 256);
	memset(outport, 0, sizeof(write_handler_t) * 256);
	interrupt_buffer = create_list();
}

ZanyCore::~ZanyCore() {
	list_free(interrupt_buffer);
}

void ZanyCore::reset() {
	this->cpu.reset();
	tstates = 0;
}

list_t *ZanyCore::get_registers() {
	static register_value_t registers[8];
	list_t *regs = create_list();
	register_value_t *cur;
	int i = 0;
	#define add_reg(ame, val) { \
		cur = &registers[i++]; \
		cur->name=ame; \
		cur->value=val;\
		list_add(regs, cur); \
	}
	add_reg("AF", cpu.getRegAF());
	add_reg("BC", cpu.getRegBC());
	add_reg("DE", cpu.getRegDE());
	add_reg("HL", cpu.getRegHL());
	add_reg("PC", cpu.getRegPC());
	add_reg("SP", cpu.getRegSP());
	add_reg("IX", cpu.getRegIX());
	add_reg("IY", cpu.getRegIY());
	return regs;
}

uint64_t ZanyCore::executedCycles() {
	return tstates;
}

void ZanyCore::execute(uint32_t cycles) {
	uint64 target_tstates = tstates + cycles;
	while (tstates < target_tstates && (!cpu.isHalted() || this->isActiveINT()))
		this->cpu.execute();
}

bool ZanyCore::loadROM(const char *path) {
	// Wait for MMU
	cpu.setHalted(true);
	while (!mmu->isReady())
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	if (mmu->loadBIOS(path)) {
		reset();
		return true;
	}
	return false;
}

void ZanyCore::attachToPort(uint32_t port, read_handler_t read_handler) {
	inport[port % 256] = read_handler;
	Log::Dbg("[ZanyCore] Input device attached to port %u (0x%x)\n", (uint16_t)port, (uint16_t)port);
}

void ZanyCore::attachToPort(uint32_t port, write_handler_t write_handler) {
	outport[port % 256] = write_handler;
	Log::Dbg("[ZanyCore] Output device attached to port %u (0x%x)\n", (uint16_t)port, (uint16_t)port);
}

void ZanyCore::fireInterrupt(uint8_t IRQ) {
	//~ if (IRQ)
		//~ Log::Dbg("Adding %d to interrupt buffer\n", IRQ);
	list_add(interrupt_buffer, new uint8_t{IRQ});
}

uint8_t ZanyCore::fetchOpcode(uint16_t address) {
	tstates++;
	uint8_t a = peek8(address);
	//Log::Dbg("Read %d at address %d\n", a, address);
	return a;
}

uint8_t ZanyCore::peek8(uint16_t address) {
	tstates += 3;
	return mmu->read(address);
}

void ZanyCore::poke8(uint16_t address, uint8_t value) {
	mmu->write(address, value);
}

uint16_t ZanyCore::peek16(uint16_t address) {
	// Little endian, so read lower byte first
	return peek8(address) | ((uint16_t)peek8(address + 1) << 8);
}

void ZanyCore::poke16(uint16_t address, uint16_t word) {
	// Little endian, so write lower byte first
	poke8(address, word & 0xFF);
	poke8(address + 1, (word & 0xFF00) >> 8);
}

/* In/Out byte from/to IO Bus */
uint8_t ZanyCore::inPort(uint16_t port) {
	port = port & 0xFF;
	uint8_t value = -1;
	if (inport[port % 256]) {
		value = inport[port]();
		//~ Log::Dbg("Reading from port %u: %u\n", port, value);
	}
	else {
		// Log::Dbg("Read from disconnected port %u\n", port);
	}
	return value;
}

void ZanyCore::outPort(uint16_t port, uint8_t value) {
	port = port & 0xFF;
	if (outport[port % 256]) {
		// Log::Dbg("Writing %u (0x%x) to port %u\n", value, value, port);
		outport[port](value);
	}
	else {
		Log::Dbg("Writing %u (0x%x) (%c) to disconnected port %u\n", value, value, value, port);
	}
}

void ZanyCore::addressOnBus(uint16_t address, int32_t wstates) {
	tstates += wstates;
}

void ZanyCore::interruptHandlingTime(uint8_t wstates) {
	tstates += wstates;
	uint8_t irq = *((uint8_t*)interrupt_buffer->items[0]);
	list_del(interrupt_buffer, 0);
	//~ if (irq) {
		//~ Log::Dbg("Handling interrupt %d\n", irq);
		//~ mmu->dump();
	//~ }
	cpu.setData(irq * 2);
}

bool ZanyCore::isActiveINT(void) {
	return interrupt_buffer->length > 0;
}

uint8_t ZanyCore::breakpoint(uint16_t address, uint8_t opcode) {
	// TODO: implement breakpoints
	return opcode;
}

void ZanyCore::execDone(void) {
	//TODO: empty stub
}
