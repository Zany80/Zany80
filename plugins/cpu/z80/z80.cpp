#define ADDRESS_BUS_SIZE 16
#define DATA_BUS_SIZE 8

#include <CPU.hpp>

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

uint16_t PC = 0;
uint8_t opcode;
word AF,BC,DE,HL;
word AF_,BC_,DE_,HL_;

uint64_t tstates = 0;
uint8_t subcycle = 0;
enum {
	INSTRUCTION_FETCH,MEM_READ,MEM_WRITE
} CPUState = INSTRUCTION_FETCH;

void executeOpcode (uint8_t opcode) {
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
