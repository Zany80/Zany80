#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <Zany80/CPU.hpp>
#include <iostream>

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
word AF,BC,DE,HL;
word AF_,BC_,DE_,HL_;

uint64_t tstates;
uint8_t subcycle;
enum {
	INSTRUCTION_FETCH,MEM_READ,MEM_WRITE
} CPUState;

void init(liblib::Library *plugin_manager) {
	CPUState = INSTRUCTION_FETCH;
	PC = 0;
	AF.word = BC.word = DE.word = HL.word = 0;
	AF_.word = BC_.word = DE_.word = HL_.word = 0;
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

void cleanup() {
	
}

void executeOpcode (uint8_t opcode) {
	std::cout << "[Z80] Executing opcode " << (int)opcode<<"\n";
	switch (opcode) {
		case 0x00:
			//nop
			break;
		case 0x01:
			break;
	}
}

void cycle() {
	if (CPUState == INSTRUCTION_FETCH) {
		subcycle++;
		if (subcycle == 1) {
			opcode = readRAM(PC++);
		}
		else if (subcycle == 4) {
			subcycle = 0;
			executeOpcode(opcode);
		}
	}
}
