#pragma once

#include <stdint.h>
#include <stddef.h>

class ZanyCore;
class MMU {
public:
	explicit MMU(ZanyCore *cpu);
	~MMU();
	bool loadBIOS(const char *path);
	void unload();
	/// Read from a virtual address
	uint8_t read(uint16_t address);
	/// Write to a virtual address
	void write(uint16_t address, uint8_t data);
	/// Translate a virtual address into a physical address
	uint32_t mapAddress(uint16_t virtual_address);
	/// Handle commands from the CPU
	void processCommand(uint8_t value);
	/// Faster read function to read a large amount of RAM at once
	void read_buf(uint32_t start_address, uint8_t *buf, size_t length);
	/**
	 * Checks if the BIOS is loaded. 
	 * @return 0 if the BIOS is still loading, 1 if the BIOS is fully loaded,
	 * or a status code
	 */
	int isReady();
	void dump();
private:
	ZanyCore *cpu;
	uint8_t memory[0x400000];
	uint8_t mapped[4];
	int status;
};

extern MMU *mmu;
