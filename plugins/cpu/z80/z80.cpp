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

uint64_t tstates;
uint8_t subcycle;

// simple buffer used to tell the MEM_READ/MEM_WRITE cycles what to do
uint32_t buffer;

enum {
	INSTRUCTION_FETCH,INSTRUCTION_EXECUTE,MEM_READ,MEM_WRITE
} CPUState;

uint16_t PC;
uint8_t opcode;
word af,bc,de,hl;
word af_,bc_,de_,hl_;

word SP;

#define S 0x80
#define Z 0x40
#define FIVE 0x20
#define H 0x10
#define THREE 0x08
#define PV 0x04
#define N 0x02
#define C 0x01

#define MEM_WRITE_FETCH 0x1000000
#define MEM_WRITE_EXECUTE 0x0000000

void setFlag(uint8_t flag,bool state) {
	if (state)
		af.l |= flag;
	else
		af.l ^= flag;
}

bool getFlag(uint8_t flag) {
	return af.l & flag;
}

void init(liblib::Library *plugin_manager) {
	CPUState = INSTRUCTION_FETCH;
	PC = 0;
	af.word = bc.word = de.word = hl.word = 0;
	af_.word = bc_.word = de_.word = hl_.word = 0;
	tstates = 0;
	subcycle = 0;
	readRAM = nullptr;
	writeRAM = nullptr;
}

inline void incR(uint8_t *r, char id) {
	uint8_t old = (*r)++;
	setFlag(S, *r & 0x80);
	setFlag(Z, *r == 0);
	setFlag(H, (old & 0x0F) == 0x0F);
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
	setFlag(H, (old & 0x1F) == 0x10);
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

inline void ldRPA(uint16_t rp, const char *id) {
	if (subcycle == 1) {
		buffer = (rp << 8) | af.h | MEM_WRITE_EXECUTE;
		CPUState = MEM_WRITE;
		subcycle++;			
	}
	else {
		std::cout << "[Z80] `ld ("<<id<<"), a` executed.\n";
		CPUState = INSTRUCTION_FETCH;
		subcycle = 0;
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

bool parity(uint8_t x){
	x ^= x >> 4;
	x ^= x >> 2;
	x ^= x >> 1;
	return x & 1;
}

inline void jrC(bool c, const char *id) {
	switch (subcycle) {
		case 1:
			CPUState = MEM_READ;
			buffer = PC++;
			subcycle++;
			break;
		case 5:
			if (!c) {
				subcycle = 0;
				CPUState = INSTRUCTION_FETCH;
				std::cout << "[Z80] `jr "<<id<<", "<<(int)((int8_t)buffer&0xFF)<<"` executed, not jumping...\n";
			}
			break;
		case 10:
			int8_t e = buffer & 0xFF;
			PC += e;
			std::cout << "[Z80] `jr "<<id<<", "<<(int)e<<"` executed...\n";
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
			break;
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
			ldRPA(bc.word, "bc");
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
		case 0x10: // djnz e
			switch (subcycle) {
				// start at 2 for djnz, as first M-state takes five cycles
				case 2:
					CPUState = MEM_READ;
					buffer = PC++;
					break;
				case 5:
					bc.h--;
					if (bc.h == 0) {
						subcycle = 0;
						CPUState = INSTRUCTION_FETCH;
						std::cout << "[Z80] `djnz "<<(int)((int8_t)(buffer & 0xFF))<<"` executed, not jumping\n";
					}
					break;
				case 10:
					int8_t e = buffer & 0xFF;
					PC += e;
					CPUState = INSTRUCTION_FETCH;
					subcycle = 0;
					std::cout << "[Z80] `djnz "<<(int)e<<"` executed.\n";
					break;
			}
			break;
		case 0x11: // ld de, **
			ldRP(&de, "de");
			break;
		case 0x12: // ld (de), a
			ldRPA(de.word, "de");
			break;
		case 0x13: // inc de
			incRP(&de.word, "de");
			break;
		case 0x14: // inc d
			incR(&de.h, 'd');
			break;
		case 0x15: // dec d
			decR(&de.h, 'd');
			break;
		case 0x16: // ld d, *
			ldR(&de.h, 'd');
			break;
		case 0x17: // rla
			{
				bool pC = getFlag(C);
				setFlag(C, af.h & 0x80);
				af.h <<= 1;
				if (pC) {
					af.h |= 0x01;
				}
			}
			setFlag(N, false);
			setFlag(H, false);
			std::cout << "[Z80] `rla` executed.\n";
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
		case 0x18: // jr e
			if (subcycle == 1) {
				buffer = PC++;
				CPUState = MEM_READ;
				subcycle++;
			}
			else {
				int8_t e = buffer & 0xFF;
				PC += e;
				subcycle = 0;
				CPUState = INSTRUCTION_FETCH;
				std::cout << "[Z80] `jr "<<(int)e<<"` executed.\n";
			}
			break;
		case 0x19: // add hl, de
			addHLSS(&de.word, "de");
			break;
		case 0x1A: // ld a, (de)
			ldARP(de.word, "de");
			break;
		case 0x1B: // dec de
			decRP(&de.word, "de");
			break;
		case 0x1C: // inc e
			incR(&de.l, 'e');
			break;
		case 0x1D: // dec e
			decR(&de.l, 'e');
			break;
		case 0x1E: // ld e, *
			ldR(&de.l, 'e');
			break;
		case 0x1F: // rra
			{
				bool pC = getFlag(C);
				setFlag(C, af.h & 0x01);
				af.h >>= 1;
				if (pC) {
					af.h |= 0x80;
				}
			}
			setFlag(H, false);
			setFlag(N, false);
			std::cout << "[Z80] `rra` executed.\n";
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
		case 0x20: // jr nz, *
			jrC(!getFlag(Z), "nz");
			break;
		case 0x21: // ld hl, **
			ldRP(&hl, "hl");
			break;
		case 0x22: // ld (**), hl
			{
				static uint16_t addr;
				switch (subcycle) {
					case 1:
						CPUState = MEM_READ;
						buffer = PC++;
						subcycle-=2;
						break;
					case 2:
						addr = buffer & 0xFF;
						CPUState = MEM_READ;
						buffer = PC++;
						break;
					case 5:
						addr |= ((buffer & 0xFF) << 8);
						buffer = (addr << 8) | hl.l | MEM_WRITE_EXECUTE;
						CPUState = MEM_WRITE;
						break;
					case 8:
						buffer = ((addr + 1) << 8) | hl.h | MEM_WRITE_EXECUTE;
						CPUState = MEM_WRITE;
						break;
					case 11:
						CPUState = INSTRUCTION_FETCH;
						subcycle = 0;
						std::cout << "[Z80] `ld ("<<addr<<"), hl` executed.\n";
						break;
				}
			}
			break;
		case 0x23: // inc hl
			incRP(&hl.word, "hl");
			break;
		case 0x24: // inc h
			incR(&hl.h, 'h');
			break;
		case 0x25: // dec h
			decR(&hl.h, 'h');
			break;
		case 0x26: // ld h, *
			ldR(&hl.h, 'h');
			break;
		case 0x27: // daa
			{
				uint8_t old = af.h;
				uint8_t v = 0;
				if ((af.h & 0xF) > 9 || getFlag(H))
					v += 0x06;
				if (((af.h+v) >> 4) > 9 || getFlag(C) || ( (uint16_t)(af.h + v) & 0x100) == 0x100)
					v += 0x60;
				if(getFlag(N)){
					af.h -= v;
					setFlag(H,((((old & 0xF) - (v & 0xF)) & 0x10) == 0x10));
				}
				else{
					af.h += v;
					setFlag(H, ((((old & 0xF) + (v & 0xF)) & 0x10) == 0x10));
				}
				setFlag(C, v >= 0x60);
			}
			setFlag(S, af.h & 0x80);
			setFlag(Z, !(bool)af.h);
			setFlag(PV, parity(af.h));
			std::cout << "[Z80] `daa` executed.";
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
		case 0x28: // jr z, *
			jrC(getFlag(Z), "z");
			break;
		case 0x29: // add hl, hl
			addHLSS(&hl.word, "hl");
			break;
		case 0x2A: // ld hl, (**)
			{
				static uint16_t addr;
				switch (subcycle) {
					case 1:
						CPUState = MEM_READ;
						buffer = PC++;
						subcycle-=2;
						break;
					case 2:
						addr = buffer & 0xFF;
						CPUState = MEM_READ;
						buffer = PC++;
						break;
					case 5:
						addr |= ((buffer & 0xFF) << 8);
						buffer = addr;
						CPUState = MEM_READ;
						break;
					case 8:
						hl.l = buffer & 0xFF;
						buffer = addr+1;
						CPUState = MEM_READ;
						break;
					case 11:
						hl.h = buffer & 0xFF;
						CPUState = INSTRUCTION_FETCH;
						subcycle = 0;
						std::cout << "[Z80] `ld hl, ("<<addr<<")` executed, value of hl: " <<(int)hl.word << "\n";
						break;
				}
			}
			break;
		case 0x2B: // dec hl
			decRP(&hl.word, "hl");
			break;
		case 0x2C: // inc l
			incR(&hl.l, 'l');
			break;
		case 0x2D: // dec l
			decR(&hl.l, 'l');
			break;
		case 0x2E: // ld l, *
			ldR(&hl.l, 'l');
			break;
		case 0x2F: // cpl
			setFlag(N, true);
			setFlag(H, true);
			af.h ^= 0xFF;
			std::cout << "[Z80] `cpl` executed.\n";
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
		default:
		case 0x00: // nop
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
	}
}

void cycle() {
	if (readRAM == nullptr || writeRAM == nullptr) {
		throw "";
	}
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
		else if (subcycle % 3 == 1) {
			if((buffer & 0x1000000) == MEM_WRITE_EXECUTE) {
				CPUState = INSTRUCTION_EXECUTE;
			}
		}
		else {
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
		}
	}
}
