#include "internals.h"

uint8_t read_platboard(limn_t *cpu, uint32_t address) {
	if (address >= 0x7FE0000) {
		// rom
		return cpu->rom->buf[address];
	}
	if (address >= 0x800 && address <= 0x804) {
		// platboard version
		// 0x00020001
		// If even, 1, if odd, 0; little endian; 00010001
		// Returning 1.1 as we don't seem to properly support 2.1 yet.
		const uint8_t addr[] = {
			1,0,1,0
		};
		return addr[address - 0x800];
	}
	if (address >= 0x1000 && address <= 0x1004) {
		return -1;
	}
    return 0;
}

void write_platboard(limn_t *cpu, uint32_t address,uint8_t value) {
	
}
