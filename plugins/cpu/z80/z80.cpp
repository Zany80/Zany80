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

inline void incR(uint8_t *r, char id) {
	uint8_t old = (*r)++;
	setFlag(S, *r & 0x80);
	setFlag(Z, *r == 0);
	setFlag(H, old & 0x0F == 0x0F);
	setFlag(N, false);
	setFlag(PV, old == 0x7F);
	subcycle = 0;
	CPUState = INSTRUCTION_FETCH;
	std::cout << "[Z80] `inc "<<id<<"` executed.\n";
}

inline void decR(uint8_t *r, char id) {
	uint8_t old = (*r)--;
	setFlag(S, *r & 0x80);
	setFlag(Z, *r == 0);
	setFlag(H, old & 0x1F == 0x10);
	setFlag(N, true);
	setFlag(PV, (old & 0x7F) == 0);
	subcycle = 0;
	CPUState = INSTRUCTION_FETCH;
	std::cout << "[Z80] `dec "<<id<<"` executed.\n";
}

inline void ldR(uint8_t *r, char id) {
	if (subcycle == 1) {
		subcycle++;
		CPUState = MEM_READ;
		buffer = PC++;
	}
	else {
		*r = buffer & 0xFF;
		std::cout << "[Z80] `ld "<<id<<", "<<(int)*r<<"` executed.\n";
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
	}
}

inline void addHLSS(uint16_t *rp, const char *id) {
	if (subcycle == 8) {
		uint32_t result = hl.word + *rp;
		setFlag(H, ((hl.word & 0xFFF) + (*rp & 0xFFF)) & 0x1000);
		setFlag(N, false);
		setFlag(C, result & 0x10000);
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
		std::cout << "[Z80] `add hl, "<<id<<"` executed.\n";
	}
}

inline void ldARP(uint16_t rp, const char *id) {
	if (subcycle == 1) {
		buffer = rp;
		CPUState = MEM_READ;
		subcycle++;
	}
	else {
		af.h = buffer & 0xFF;
		CPUState = INSTRUCTION_FETCH;
		subcycle = 0;
		std::cout << "[Z80] `ld a, ("<<id<<")` executed.\n";
	}
}

inline void ldRP(word *rp, const char *id) {
	switch (subcycle) {
		case 1:
			CPUState = MEM_READ;
			subcycle-=2;
			buffer = PC++;
			break;
		case 2:
			rp->l = buffer & 0xFF;
			CPUState = MEM_READ;
			buffer = PC++;
			break;
		case 5:
			rp->h = buffer & 0xFF;
			subcycle = 0;
			std::cout << "[Z80] `ld "<<id<<", "<<(int)rp->word<<"` executed.\n";
			CPUState = INSTRUCTION_FETCH;
			break;
	}
}

inline void incRP(uint16_t *rp,const char *id) {
	if (subcycle == 3) {
		(*rp)++;
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
		std::cout << "[Z80] `inc "<<id<<"` executed.\n";
	}
}

inline void decRP(uint16_t *rp, const char *id) {
	if (subcycle == 3) {
		(*rp)--;
		CPUState = INSTRUCTION_FETCH;
		subcycle = 0;
		std::cout << "[Z80] `dec "<<id<<"` executed.\n";
	}
}

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
			ldRP(&bc, "bc");
			break;
		case 0x02: // ld (bc), a
			buffer = (bc.word << 8) | af.h;
			CPUState = MEM_WRITE;
			subcycle++;
			std::cout << "[Z80] `ld (bc), a` executed.\n";
			break;
		case 0x03: // inc bc
			//takes 6 cycle. execute is first called on the fourth cycle with subcycle set to 1.
			incRP(&bc.word, "bc");
			break;
		case 0x04: // inc b
			incR(&bc.h, 'b');
			break;
		case 0x05: // dec b
			decR(&bc.h, 'b');
			break;
		case 0x06: // ld b, *
			ldR(&bc.h,'b');
			break;
		case 0x07: // rlca
			setFlag(C, af.h & 0x80);
			af.h <<= 1;
			if (getFlag(C))
				af.h |= 0x01;
			setFlag(N, false);
			setFlag(H, false);
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			std::cout << "[Z80] `rlca` executed.\n";
			break;
		case 0x08: // ex af, af'
			{
				uint16_t t = af.word;
				af.word = af_.word;
				af_.word = t;
			}
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			std::cout << "[Z80] `ex af, af'` executed.\n";
			break;
		case 0x09: // add hl, bc
			addHLSS(&bc.word,"bc");
			break;
		case 0x0A: // ld a, (bc)
			ldARP(bc.word,"bc");
			break;
		case 0x0B: // dec bc
			decRP(&bc.word, "bc");
			break;
		case 0x0C: // inc c
			incR(&bc.l, 'c');
			break;
		case 0x0D: // dec c
			decR(&bc.l, 'c');
			break;
		case 0x0E: // ld c, *
			ldR(&bc.l, 'c');
			break;
		case 0x0F: // rrca
			setFlag(C, af.h & 0x01);
			af.h >>= 1;
			if (getFlag(C)) {
				af.h |= 0x80;
			}
			setFlag(N, false);
			setFlag(H, false);
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			std::cout << "[Z80] `rrca` executed.\n";
			break;
		default:
		case 0x00: // nop
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
			if (buffer & 0x10000) {
				CPUState = INSTRUCTION_EXECUTE;
			}
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
