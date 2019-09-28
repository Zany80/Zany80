#include "MMU.h"
#include "ZanyCore.h"

#include <stdio.h>
#include <string.h>

#include <Zany80/API.h>

MMU::MMU(ZanyCore *cpu) {
	this->cpu = cpu;
	this->status = -1;
	cpu->attachToPort(0, [](uint32_t value) {
		mmu->processCommand(value & 0xFF);
	});
	//~ for (size_t i = 0; i < 4; i++) {
		//~ for (size_t j = 0; j < 0x4000; j++) {
			//~ Log::Dbg("0x%x maps to 0x%x\n", j + 0x4000 * i, mapAddress(j + 0x4000 * i));
			//~ o_assert_dbg(j + 0x4000 * mapped[i] == mapAddress(j + 0x4000 * i));
		//~ }
	//~ }
}

MMU::~MMU() {

}

bool MMU::loadBIOS(const char *path) {
	this->status = 0;
	FILE *file = fopen(path, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		printf("Loading rom of size: %ld\n", size);
		if (size > 0x4000)
			size = 0x4000;
		rewind(file);
		fread(memory, 1, size, file);
		fclose(file);
		zany_log(ZL_INFO, "[MMU] BIOS Loaded\n");
		for (uint8_t i = 0; i < 4; i++) {
			this->mapped[i] = i;
		}
		this->status = 1;
		return true;
	}
	else {
		this->status = 2;
		zany_log(ZL_ERROR, "[z80cpp_core/MMU] Error loading BIOS\n");
		//~ zany_report_error("[z80cpp_core/MMU] Error loading BIOS");
		return false;
	}
}

void MMU::unload() {
	this->status = 255;
}

void MMU::processCommand(uint8_t value) {
	static enum {
		ready, map, 
		//~ adjust_protection
	} state = ready;
	static uint8_t new_mapping = 0;
	switch (state) {
		case ready:
			if (value < 4) {
				new_mapping = value;
				state = map;
			}
			else if ((value & 0xE0) == 0) {
				// Since this is a non-zero value, some bits from 2-4
				//~ new_mapping = (value & 0x1C) >> 2;
				//~ state = adjust_protection;
				zany_log(ZL_INFO, "[MMU] Memory protection not yet implemented");
				
			}
			else if (value == 0xFF) {
				dump();
			}
			break;
		case map:
			state = ready;
			mapped[new_mapping] = value;
			zany_log(ZL_INFO, "[MMU] Mapped page %u (0x%x) into %u\n", value, value, new_mapping);
			break;
	}
}

uint32_t MMU::mapAddress(uint16_t virtual_address) {
	return (mapped[(virtual_address & 0xC000) >> 14] * 0x4000) | (virtual_address & 0x3FFF);
}

// TODO: per-page read/write/execute protection

uint8_t MMU::read(uint16_t address) {
	return memory[mapAddress(address)];
}

void mini_gpu(uint16_t address, uint8_t value);

void MMU::write(uint16_t address, uint8_t data) {
	int page = mapped[(address & 0xC000) >> 14];
	// redirect writes to simple "video card"
	if (page == 255) {
		mini_gpu(address & 0x3FFF, data);
	}
	// protect ROM
	else if (page != 0) {
		memory[mapAddress(address)] = data;
	}
}

int MMU::isReady() {
	return status;
}

void MMU::dump() {
	static int index = 0;
	char buf[16];
	sprintf(buf, "dump%d.rom", index++);
	FILE *dump = fopen(buf, "wb");
	if (dump) {
		fwrite(memory, 0x4000, 256, dump);
		fclose(dump);
	}
}

void MMU::read_buf(uint32_t start_address, uint8_t *buf, size_t length) {
	memcpy(buf, memory + start_address, length);
}

MMU *mmu;
