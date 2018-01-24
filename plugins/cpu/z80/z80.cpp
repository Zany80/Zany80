#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <Zany80/CPU.hpp>
#include <iostream>

#include <SFML/System.hpp>

const char *signature = "z80";

const char *neededPlugins() {
	return "RAM/16_8";
}

typedef union{
	uint16_t word;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	struct{
		uint8_t l,h;
	};
#else
	struct{
		uint8_t h,l;
	};
#endif
} word;

uint16_t PC;
uint8_t opcode;
word af,bc,de,hl;
word af_,bc_,de_,hl_;

#define S 0x80
#define Z 0x40
#define FIVE 0x20
#define H 0x10
#define THREE 0x08
#define PV 0x04
#define N 0x02
#define C 0x01

void setFlag(uint8_t flag,bool state) {
	if (state)
		af.l |= flag;
	else
		af.l ^= flag;
}

bool getFlag(uint8_t flag) {
	return af.l & flag;
}

uint64_t tstates;
uint8_t subcycle;

// simple buffer used to tell the MEM_READ/MEM_WRITE cycles what to do
uint32_t buffer;

enum {
	INSTRUCTION_FETCH,INSTRUCTION_EXECUTE,MEM_READ,MEM_WRITE
} CPUState;

void init(liblib::Library *plugin_manager) {
	CPUState = INSTRUCTION_FETCH;
	PC = 0;
	af.word = bc.word = de.word = hl.word = 0;
	af_.word = bc_.word = de_.word = hl_.word = 0;
	tstates = 0;
	subcycle = 0;
	liblib::Library *ram = ((liblib::Library *(*)(const char *))(*plugin_manager)["getRAM"])("16_8");
	if (ram != nullptr) {
		setRAM(ram);
	}
	else {
		throw std::exception();
	}
}

uint64_t *getCycles() {
	return &tstates;
}

void cleanup() {
	
}

void executeOpcode (uint8_t opcode) {
	std::cout << "[Z80]  Executing opcode " << (int)opcode<<" at cycle "<<(int)tstates<<"\n";
	switch (opcode) {
		case 0x01: // ld bc, **
			switch (subcycle) {
				case 1:
					CPUState = MEM_READ;
					subcycle-=2;
					buffer = PC++;
					break;
				case 2:
					bc.l = buffer & 0xFF;
					CPUState = MEM_READ;
					buffer = PC++;
					break;
				case 5:
					bc.h = buffer & 0xFF;
					subcycle = 0;
					std::cout << "[Z80] `ld bc, "<<(int)bc.word<<"` executed.\n";
					CPUState = INSTRUCTION_FETCH;
					break;
			}
			break;
		case 0x02: // ld (bc), a
			buffer = (bc.word << 8) | af.h;
			CPUState = MEM_WRITE;
			subcycle++;
			std::cout << "[Z80] `ld (bc), a` executed.\n";
			break;
		case 0x03: // inc bc
			//takes 6 cycle. execute is first called on the fourth cycle with subcycle set to 1.
			if (subcycle == 3) {
				bc.word++;
				subcycle = 0;
				CPUState = INSTRUCTION_FETCH;
				std::cout << "[Z80] `inc bc` executed.\n";
			}
			else {
				std::cout << "[Z80] "<<(3-subcycle)<<" cycles remaining for `inc bc`...\n";
			}
			break;
		case 0x04: // inc b
			{
				uint8_t old = bc.h++;
				setFlag(S, bc.h & 0x80);
				setFlag(Z, bc.h == 0);
				setFlag(H, old & 0x0F == 0x0F);
				setFlag(N, false);
				setFlag(PV, old == 0x7F);
			}
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			std::cout << "[Z80] `inc b` executed.\n";
			break;
		default:
		case 0x00: //nop
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
	}
}

void cycle() {
	tstates++;
	subcycle++;
	if (CPUState == INSTRUCTION_FETCH) {
		if (subcycle == 1) {
			opcode = readRAM(PC++);
		}
		else if (subcycle == 3) {
			subcycle = 0;
			CPUState = INSTRUCTION_EXECUTE;
		}
	}
	else if (CPUState == INSTRUCTION_EXECUTE) {
		executeOpcode(opcode);
	}
	else if (CPUState == MEM_READ) {
		if (subcycle % 3 == 0) {
			bool skip_delay = buffer & 0x10000;
			buffer = readRAM(buffer & 0xFFFF);
			//if (buffer & 0x10000) {
				//CPUState = INSTRUCTION_EXECUTE;
			//}
		}
		// The execute cycle is the first of the three cycles for reading memory, so only delay one cycle here
		else if (subcycle % 3 == 1) {
			CPUState = INSTRUCTION_EXECUTE;
		}
	}
	else if (CPUState == MEM_WRITE) {
		if (subcycle %3 == 0) {
			uint16_t address = (buffer & 0xFFFF00) >> 8;
			uint8_t value = buffer & 0xFF;
			writeRAM(address,value);
			// not needed yet, left here b/c it will be soon
			//if (buffer & 0x1000000) {
				//CPUState = (buffer & 0x2000000) ? INSTRUCTION_EXECUTE : INSTRUCTION_FETCH;
			//}
		}
		else if (subcycle % 3 == 2) {
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
		}
	}
}
