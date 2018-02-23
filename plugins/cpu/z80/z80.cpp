#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <Zany80/Zany80.hpp>

#include <Zany80/CPU.hpp>
#include <iostream>

#include <SFML/System.hpp>

liblib::Library *pm;

const char *signature = "z80";

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

word SP;

const char *neededPlugins() {
	return "RAM/16_8";
}

void reset();

void postMessage(PluginMessage m) {
	if (strcmp(m.data, "reset") == 0) {
		reset();
	}
	else if (strcmp(m.data, "setPC") == 0) {
		PC = *(uint16_t*)m.context;
	}
}

uint64_t tstates;
uint8_t subcycle;

// simple buffer used to tell the MEM_READ/MEM_WRITE cycles what to do
uint32_t buffer;

enum {
	HALTED,INSTRUCTION_FETCH,INSTRUCTION_EXECUTE,MEM_READ,MEM_WRITE
} CPUState;

void reset() {
	try {
		((textMessage_t)(*pm)["textMessage"])("boot_flash","CPU/z80;Runner/ROM");
	}
	catch (std::exception){}
	CPUState = INSTRUCTION_FETCH;
	PC = 0;
}

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
	pm = plugin_manager;
	CPUState = HALTED;
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
	//std::cout << "[Z80] `inc "<<id<<"` executed.\n";
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
	//std::cout << "[Z80] `dec "<<id<<"` executed.\n";
}

inline void ldR(uint8_t *r, char id) {
	if (subcycle == 1) {
		subcycle++;
		CPUState = MEM_READ;
		buffer = PC++;
	}
	else {
		*r = buffer & 0xFF;
		//std::cout << "[Z80] `ld "<<id<<", "<<(int)*r<<"` executed.\n";
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
	}
}

inline void ldRRx(uint8_t *r, uint8_t *rx, char id, char idx) {
	*r = *rx;
	// std::cout << "[Z80] `ld "<<id<<", "<<idx<<"` executed.\n";
	CPUState = INSTRUCTION_FETCH;
	subcycle = 0;
}

inline void ldR_HL_(uint8_t *r, char id) {
	switch (subcycle) {
		case 1:
			CPUState = MEM_READ;
			buffer = hl.word;
			subcycle -= 2;
			break;
		case 2:
			*r = buffer & 0xFF;
			//std::cout << "[Z80] `ld "<<id<<", (hl)` executed.\n";
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
			break;
	}
}

inline void ld_HL_R(uint8_t r, char id) {
	CPUState = MEM_WRITE;
	buffer = (hl.word << 8) | r | MEM_WRITE_EXECUTE;
	subcycle++;
}

inline void addA(uint8_t value) {
	uint16_t result = af.h + value;
	uint8_t r = result & 0xFF;
	setFlag(S, r & 0x80);
	setFlag(Z, !r);
	setFlag(H, ((af.h & 0x0F) + (value & 0x0F)) & 0x10 == 0x10);
	setFlag(PV, (af.h & 0x80) == (value & 0x80) && (value & 0x80) != (r & 0x80));
	setFlag(N, false);
	setFlag(C, result & 0x100);
	af.h = r;
	subcycle = 0;
	CPUState = INSTRUCTION_FETCH;
}

inline void addAR(uint8_t value, char id) {
	addA(value);
}

inline void adcAR(uint8_t value, char id) {
	addA(value + getFlag(C));
}

inline void addHLSS(uint16_t *rp, const char *id) {
	if (subcycle == 8) {
		uint32_t result = hl.word + *rp;
		setFlag(H, ((hl.word & 0xFFF) + (*rp & 0xFFF)) & 0x1000);
		setFlag(N, false);
		setFlag(C, result & 0x10000);
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
		//std::cout << "[Z80] `add hl, "<<id<<"` executed.\n";
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
		//std::cout << "[Z80] `ld a, ("<<id<<")` executed.\n";
	}
}

inline void ldRPA(uint16_t rp, const char *id) {
	if (subcycle == 1) {
		buffer = (rp << 8) | af.h | MEM_WRITE_EXECUTE;
		CPUState = MEM_WRITE;
		subcycle++;			
	}
	else {
		//std::cout << "[Z80] `ld ("<<id<<"), a` executed.\n";
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
			//std::cout << "[Z80] `ld "<<id<<", "<<(int)rp->word<<"` executed.\n";
			CPUState = INSTRUCTION_FETCH;
			break;
	}
}

inline void incRP(uint16_t *rp,const char *id) {
	if (subcycle == 3) {
		(*rp)++;
		subcycle = 0;
		CPUState = INSTRUCTION_FETCH;
		//std::cout << "[Z80] `inc "<<id<<"` executed.\n";
	}
}

inline void decRP(uint16_t *rp, const char *id) {
	if (subcycle == 3) {
		(*rp)--;
		CPUState = INSTRUCTION_FETCH;
		subcycle = 0;
		//std::cout << "[Z80] `dec "<<id<<"` executed.\n";
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
				//std::cout << "[Z80] `jr "<<id<<", "<<(int)((int8_t)buffer&0xFF)<<"` executed, not jumping...\n";
			}
			break;
		case 10:
			int8_t e = buffer & 0xFF;
			PC += e;
			//std::cout << "[Z80] `jr "<<id<<", "<<(int)e<<"` executed...\n";
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
			break;
	}
}

uint64_t *getCycles() {
	return &tstates;
}

//TODO: remove cleanup/init function from all plugins, use message system instead. Bonus points if it works multithreaded ;)
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
			//std::cout << "[Z80] `rlca` executed.\n";
			break;
		case 0x08: // ex af, af'
			{
				uint16_t t = af.word;
				af.word = af_.word;
				af_.word = t;
			}
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			//std::cout << "[Z80] `ex af, af'` executed.\n";
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
			//std::cout << "[Z80] `rrca` executed.\n";
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
						//std::cout << "[Z80] `djnz "<<(int)((int8_t)(buffer & 0xFF))<<"` executed, not jumping\n";
					}
					break;
				case 10:
					int8_t e = buffer & 0xFF;
					PC += e;
					CPUState = INSTRUCTION_FETCH;
					subcycle = 0;
					//std::cout << "[Z80] `djnz "<<(int)e<<"` executed.\n";
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
			//std::cout << "[Z80] `rla` executed.\n";
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
				//std::cout << "[Z80] `jr "<<(int)e<<"` executed.\n";
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
			//std::cout << "[Z80] `rra` executed.\n";
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
						//std::cout << "[Z80] `ld ("<<addr<<"), hl` executed.\n";
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
			//std::cout << "[Z80] `daa` executed.";
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
						//std::cout << "[Z80] `ld hl, ("<<addr<<")` executed, value of hl: " <<(int)hl.word << "\n";
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
			//std::cout << "[Z80] `cpl` executed.\n";
			subcycle = 0;
			CPUState = INSTRUCTION_FETCH;
			break;
		case 0x30: // jr nc, *
			jrC(!getFlag(C), "nc");
			break;
		case 0x31: // ld sp, **
			ldRP(&SP, "sp");
			break;
		case 0x32: // ld (**), a
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
						buffer = (addr << 8) | af.h | MEM_WRITE_EXECUTE;
						CPUState = MEM_WRITE;
						break;
					case 8:
						CPUState = INSTRUCTION_FETCH;
						subcycle = 0;
						//std::cout << "[Z80] `ld ("<<addr<<"), a` executed.\n";
						break;
				}
			}
			break;
		case 0x33: // inc sp
			incRP(&SP.word, "sp");
			break;
		case 0x34: // inc (hl)
			switch (subcycle) {
				static uint8_t old;
				case 1:
					CPUState = MEM_READ;
					buffer = hl.word;
					subcycle-=2;
					break;
				// takes *four* cycles to read+increment (as opposed to the usual 3)
				case 3:
					old = buffer & 0xFF;
					buffer = (hl.word << 8) | (old+1) | MEM_WRITE_EXECUTE;
					CPUState = MEM_WRITE;
					subcycle--;
					break;
				case 5:
					setFlag(S, (old+1) & 0x80);
					setFlag(Z, (old+1) == 0);
					setFlag(H, (old & 0x0F) == 0x0F);
					setFlag(PV, old == 0x7F);
					setFlag(N, false);
					//std::cout << "[Z80]`inc (hl)` executed.\n";
					CPUState = INSTRUCTION_FETCH;
					subcycle = 0;
					break;
			}
			break;
		case 0x35: // dec (hl)
		switch (subcycle) {
				static uint8_t old;
				case 1:
					CPUState = MEM_READ;
					buffer = hl.word;
					subcycle-=2;
					break;
				// takes *four* cycles to read+decrement (as opposed to the usual 3)
				case 3:
					old = buffer & 0xFF;
					buffer = (hl.word << 8) | (old-1) | MEM_WRITE_EXECUTE;
					CPUState = MEM_WRITE;
					subcycle--;
					break;
				case 5:
					setFlag(S, (old-1) & 0x80);
					setFlag(Z, (old-1) == 0);
					setFlag(H, (old & 0x0F) == 0x00);
					setFlag(PV, old == 0x80);
					setFlag(N, false);
					//std::cout << "[Z80]`dec (hl)` executed.\n";
					CPUState = INSTRUCTION_FETCH;
					subcycle = 0;
					break;
			}
			break;
		case 0x36: // ld (hl), *
			switch (subcycle) {
				static uint8_t value;
				case 1:
					CPUState = MEM_READ;
					buffer = PC++;
					subcycle-=2;
					break;
				case 2:
					CPUState = MEM_WRITE;
					value = buffer & 0xFF;
					buffer = (hl.word << 8) | (value) | MEM_WRITE_EXECUTE;
					break;
				case 5:
					//std::cout << "[Z80] `ld (hl), "<<(int)value<<"` executed.\n";
					CPUState = INSTRUCTION_FETCH;
					subcycle = 0;
					break;
			}
			break;
		case 0x37: // scf
			setFlag(C, true);
			setFlag(N, false);
			setFlag(H, false);
			//std::cout << "[Z80] `scf` executed.\n";
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
			break;
		case 0x38: // jr c, *
			jrC(getFlag(C),"c");
			break;
		case 0x39: // add hl, sp
			addHLSS(&SP.word, "sp");
			break;
		case 0x3A: // ld a, (**)
			switch (subcycle) {
				static uint16_t addr;
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
					addr |= (buffer & 0xFF) << 8;
					CPUState = MEM_READ;
					buffer = addr;
					break;
				case 8:
					af.h = buffer & 0xFF;
					//std::cout << "[Z80] `ld a, ("<<(int)addr<<")` executed.\n";
					subcycle = 0;
					CPUState = INSTRUCTION_FETCH;
					break;
			}
			break;
		case 0x3B: // dec sp
			decRP(&SP.word, "sp");
			break;
		case 0x3C: // inc a
			incR(&af.h, 'a');
			break;
		case 0x3D: // dec a
			decR(&af.h, 'a');
			break;
		case 0x3E: // ld a, *
			ldR(&af.h, 'a');
			break;
		case 0x3F: // ccf
			setFlag(H, getFlag(C));
			setFlag(C, !getFlag(C));
			setFlag(N, false);
			//std::cout << "[Z80] `ccf` executed.\n";
			CPUState = INSTRUCTION_FETCH;
			subcycle = 0;
			break;
		case 0x40: // ld b, b
			ldRRx(&bc.h, &bc.h, 'b', 'b');
			break;
		case 0x41: // ld b, c
			ldRRx(&bc.h, &bc.l, 'b', 'c');
			break;
		case 0x42: // ld b, d
			ldRRx(&bc.h, &de.h, 'b', 'd');
			break;
		case 0x43: // ld b, e
			ldRRx(&bc.h, &de.l, 'b', 'e');
			break;
		case 0x44: // ld b, h
			ldRRx(&bc.h, &hl.h, 'b', 'h');
			break;
		case 0x45: // ld b, l
			ldRRx(&bc.h, &hl.l, 'b', 'l');
			break;
		case 0x46: // ld b, (hl)
			ldR_HL_(&bc.h, 'b');
			break;
		case 0x47: // ld b, a
			ldRRx(&bc.h, &af.h, 'b', 'a');
			break;
		case 0x48: // ld c, b
			ldRRx(&bc.l, &bc.h, 'c', 'b');
			break;
		case 0x49: // ld c, c
			ldRRx(&bc.l, &bc.l, 'c', 'c');
			break;
		case 0x4A: // ld c, d
			ldRRx(&bc.l, &de.h, 'c', 'd');
			break;
		case 0x4B: // ld c, e
			ldRRx(&bc.l, &de.l, 'c', 'e');
			break;
		case 0x4C: // ld c, h
			ldRRx(&bc.l, &hl.h, 'c', 'h');
			break;
		case 0x4D: // ld c, l
			ldRRx(&bc.l, &hl.l, 'c', 'l');
			break;
		case 0x4E: // ld c, (hl)
			ldR_HL_(&bc.l, 'c');
			break;
		case 0x4F: // ld c, a
			ldRRx(&bc.l, &af.h, 'c', 'a');
			break;
		case 0x50: // ld d, b
			ldRRx(&de.h, &bc.h, 'd', 'b');
			break;
		case 0x51: // ld d, c
			ldRRx(&de.h, &bc.l, 'd', 'c');
			break;
		case 0x52: // ld d, d
			ldRRx(&de.h, &de.h, 'd', 'd');
			break;
		case 0x53: // ld d, e
			ldRRx(&de.h, &de.l, 'd', 'e');
			break;
		case 0x54: // ld d, h
			ldRRx(&de.h, &hl.h, 'd', 'h');
			break;
		case 0x55: // ld d, l
			ldRRx(&de.h, &hl.l, 'd', 'l');
			break;
		case 0x56: // ld d, (hl)
			ldR_HL_(&de.h, 'd');
			break;
		case 0x57: // ld d, a
			ldRRx(&de.h, &af.h, 'd', 'a');
			break;
		case 0x58: // ld e, b
			ldRRx(&de.l, &bc.h, 'e', 'b');
			break;
		case 0x59: // ld e, c
			ldRRx(&de.l, &bc.l, 'e', 'c');
			break;
		case 0x5A: // ld e, d
			ldRRx(&de.l, &de.h, 'e', 'd');
			break;
		case 0x5B: // ld e, e
			ldRRx(&de.l, &de.l, 'e', 'e');
			break;
		case 0x5C: // ld e, h
			ldRRx(&de.l, &hl.h, 'e', 'h');
			break;
		case 0x5D: // ld e, l
			ldRRx(&de.l, &hl.l, 'e', 'l');
			break;
		case 0x5E: // ld e, (hl)
			ldR_HL_(&de.l, 'e');
			break;
		case 0x5F: // ld e, a
			ldRRx(&de.l, &af.h, 'e', 'a');
			break;
		case 0x60: // ld h, b
			ldRRx(&hl.h, &bc.h, 'h', 'b');
			break;
		case 0x61: // ld h, c
			ldRRx(&hl.h, &bc.l, 'h', 'c');
			break;
		case 0x62: // ld h, d
			ldRRx(&hl.h, &de.h, 'h', 'd');
			break;
		case 0x63: // ld h, e
			ldRRx(&hl.h, &de.l, 'h', 'e');
			break;
		case 0x64: // ld h, h
			ldRRx(&hl.h, &hl.h, 'h', 'h');
			break;
		case 0x65: // ld h, l
			ldRRx(&hl.h, &hl.l, 'h', 'l');
			break;
		case 0x66: // ld h, (hl)
			ldR_HL_(&hl.h, 'd');
			break;
		case 0x67: // ld h, a
			ldRRx(&hl.h, &af.h, 'h', 'a');
			break;
		case 0x68: // ld l, b
			ldRRx(&hl.l, &bc.h, 'l', 'b');
			break;
		case 0x69: // ld l, c
			ldRRx(&hl.l, &bc.l, 'l', 'c');
			break;
		case 0x6A: // ld l, d
			ldRRx(&hl.l, &de.h, 'l', 'd');
			break;
		case 0x6B: // ld l, e
			ldRRx(&hl.l, &de.l, 'l', 'e');
			break;
		case 0x6C: // ld l, h
			ldRRx(&hl.l, &hl.h, 'l', 'h');
			break;
		case 0x6D: // ld l, l
			ldRRx(&hl.l, &hl.l, 'l', 'l');
			break;
		case 0x6E: // ld l, (hl)
			ldR_HL_(&hl.l, 'l');
			break;
		case 0x6F: // ld l, a
			ldRRx(&hl.l, &af.h, 'l', 'a');
			break;
		case 0x70: // ld (hl), b
			ld_HL_R(bc.h, 'b');
			break;
		case 0x71: // ld (hl), c
			ld_HL_R(bc.l, 'b');
			break;
		case 0x72: // ld (hl), d
			ld_HL_R(de.h, 'b');
			break;
		case 0x73: // ld (hl), e
			ld_HL_R(de.l, 'b');
			break;
		case 0x74: // ld (hl), h
			ld_HL_R(hl.h, 'b');
			break;
		case 0x75: // ld (hl), l
			ld_HL_R(hl.l, 'b');
			break;
		case 0x76: // halt
			CPUState = HALTED;
			break;
		case 0x77: // ld (hl), a
			ld_HL_R(af.h, 'a');
			break;
		case 0x78: // ld a, b
			ldRRx(&af.h, &bc.h, 'a', 'b');
			break;
		case 0x79: // ld a, c
			ldRRx(&af.h, &bc.l, 'a', 'c');
			break;
		case 0x7A: // ld a, d
			ldRRx(&af.h, &de.h, 'a', 'd');
			break;
		case 0x7B: // ld a, e
			ldRRx(&af.h, &de.l, 'a', 'e');
			break;
		case 0x7C: // ld a, h
			ldRRx(&af.h, &hl.h, 'a', 'h');
			break;
		case 0x7D: // ld a, l
			ldRRx(&af.h, &hl.l, 'a', 'l');
			break;
		case 0x7E: // ld a, (hl)
			ldR_HL_(&af.h, 'a');
			break;
		case 0x7F: // ld a, a
			ldRRx(&af.h, &af.h, 'a', 'a');
			break;
		case 0x80: // add a, b
			addAR(bc.h, 'b');
			break;
		case 0x81: // add a, c
			addAR(bc.l, 'c');
			break;
		case 0x82: // add a, d
			addAR(de.h, 'd');
			break;
		case 0x83: // add a, e
			addAR(de.l, 'e');
			break;
		case 0x84: // add a, h
			addAR(hl.h, 'h');
			break;
		case 0x85: // add a, l
			addAR(hl.l, 'l');
			break;
		case 0x86: // add a, (hl)
			if (subcycle == 1) {
				CPUState = MEM_READ;
				buffer = hl.word;
				subcycle -= 2;
			}
			else {
				addA(buffer & 0xFF);
				subcycle = 0;
				CPUState = INSTRUCTION_FETCH;
			}
			break;
		case 0x87: // add a, a
			addAR(af.h, 'a');
			break;
		case 0x88: // adc a, b
			adcAR(bc.h, 'b');
			break;
		case 0x89: // adc a, c
			adcAR(bc.l, 'c');
			break;
		case 0x8A: // adc a, d
			adcAR(de.h, 'd');
			break;
		case 0x8B: // adc a, e
			adcAR(de.l, 'e');
			break;
		case 0x8C: // adc a, h
			adcAR(hl.h, 'h');
			break;
		case 0x8D: // adc a, l
			adcAR(hl.l, 'l');
			break;
		case 0x8E: // adc a, (hl)
			if (subcycle == 1) {
				CPUState = MEM_READ;
				buffer = hl.word;
				subcycle -= 2;
			}
			else {
				addA(buffer & 0xFF + getFlag(C));
				subcycle = 0;
				CPUState = INSTRUCTION_FETCH;
			}
			break;
		case 0x8F: // adc a, a
			adcAR(af.h, 'a');
			break;
		
		case 0xD3: // out (*), a
			switch (subcycle) {
			case 1:
				CPUState = MEM_READ;
				subcycle -= 2;
				buffer = PC++;
				break;
			case 6:
				// prevent A from being modified
				uint8_t backup = af.h;
				((broadcast_t)(*pm)["broadcast"])({
					(int)buffer & 0xFF, (char *)&af.h, 1, "CPU/z80", "output"
				});
				af.h = backup;
				CPUState = INSTRUCTION_FETCH;
				subcycle = 0;
				break;
			}
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
		throw;
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
	else if (CPUState == HALTED) {
		executeOpcode(0);
		/*  NOP sets states to INSTRUCTION_EXECUTE; setting here vs if statement there
		 * Checking if it's halted every time there is an extra jump every cycle when not halted
		 * this is a single assignment when halted
		 * microoptimizing! Yay! */
		CPUState = HALTED;
	}
}
