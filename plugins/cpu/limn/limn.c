#include "internals.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Zany80/API.h>
#include <stretchy_buffer.h>

#define R(r) cpu->registers[r]
#define KERNEL_MODE ((R(LIMN_RS) & 0x01) == 0)
#define USER_MODE ((R(LIMN_RS) & 0x01) == 1)
#define REG_ALLOWED(r) (r < 32 || KERNEL_MODE)
#define FORBIDDEN cpu->running = false;LIMN_LOG(cpu, ZL_ERROR, "Access denied! Attempted: "
#define ALLOWED LIMN_LOG(cpu, ZL_DEBUG,
#define BUSERR ALLOWED "BUS ERROR!\n");dump_rom(cpu, "buserred.rom");LIMN_LOG(cpu, ZL_ERROR, "Opcode: %d\n\t",current_opcode);LIMN_LOG(cpu, ZL_DEBUG,"\n\n!!! BUSERR at %.8x !!!\n\n", address);dump_backtrace(cpu);
#define CUR_STACK (KERNEL_MODE ? LIMN_SP : LIMN_USP)
#define CALL(addr) {sb_push(cpu->call_stack, R(LIMN_PC));push(cpu, CUR_STACK, R(LIMN_PC));R(LIMN_PC) = addr;}
#define BRANCH(condition, str) {\
                uint32_t imm = pc_long(cpu);\
            if (condition) {\
                if (debuggerize) {\
                    ALLOWED "Branching to %.8x; condition: " str "; RF: %.8x\n", imm, R(LIMN_RF));\
                }\
                R(LIMN_PC) = imm;\
            }\
            else {\
                if (debuggerize) {\
                    ALLOWED "Not branching to %.8x; condition: " str "\n", imm);\
                }\
            }\
}

static bool branches = false;

void dump_rom(limn_t *cpu, const char *path) {
	FILE *f = fopen(path, "wb");
	if (f) {
		fwrite(cpu->ram, 1, LIMN_RAM_SIZE, f);
		fflush(f);
		fclose(f);
		if (cpu->serial.write) {
			const char *dumped = "\nROM dumped\n";
			for (size_t i = 0; i < strlen(dumped); i++) {
				cpu->serial.write(dumped[i]);
			}
		}
	}
}

uint8_t read_ram_board(limn_t *cpu, uint32_t address) {
    switch (address) {
        case 0:
            return 1;
        case 6:
            return 0x10;
        default:
            return 0;
    }
}

uint8_t read_rom(limn_t *cpu, uint32_t address) {
    return address < cpu->rom->rom_size ? cpu->rom->buf[address] : -1;
}

uint8_t read_ram(limn_t *cpu, uint32_t address) {
    return cpu->ram[address];
}

void write_ram(limn_t *cpu, uint32_t address, uint8_t value) {
    cpu->ram[address] = value;
}

static bool debuggerize = false;

void LIMN_LOG(limn_t *cpu, zany_loglevel level, const char *format, ...) {
	va_list args;
	va_start(args, format);
    char buf[1024];
	vsprintf(buf, format, args);
	va_end(args);
    if (debuggerize) {
		zany_log(level, "%s", buf);
	}
    if (cpu && cpu->serial.write) {
        for (size_t i = 0; i < strlen(buf); i++) {
            cpu->serial.write(buf[i]);
        }
    }
}

void dump_backtrace(limn_t *cpu) {
	sb_push(cpu->call_stack, R(LIMN_PC));
	bool p_branch = branches;
	branches = false;
	for (int i = sb_count(cpu->call_stack) - 1; i >= 0; i--) {
		ALLOWED "%d: %.8x\n", i, cpu->call_stack[i]);
	}
	sb_pop(cpu->call_stack, 1);
	ALLOWED "\n");
	branches = p_branch;
}

void serial_handler(limn_t *cpu, uint8_t command) {
    switch (command) {
        case LIMN_SERIAL_WRITE:{
			bool p_d = debuggerize;
			debuggerize = true;
            ALLOWED "%c", cpu->serial.current_data);
            debuggerize = p_d;}
            break;
        case LIMN_SERIAL_READ:{
            uint32_t value = 0xFFFF;
            if (cpu->serial.read) {
                value = cpu->serial.read();
            }
            cpu->serial.current_data = value & 0xFFFF;
            }break;
        default:
            LIMN_LOG(cpu,ZL_ERROR, "Serial controller received unknown command %d\n", command);
            break;
    }
}

void write_serial(limn_t *cpu, uint32_t address, uint8_t value) {
    switch (address) {
        case 0:
            // Command port
            serial_handler(cpu, value);
            break;
        case 4:
            // Data port
            cpu->serial.current_data = value;
            break;
        default:
            zany_log(ZL_DEBUG, "Write to unexpected address in serial chip\n");
            break;
    }
}

uint8_t read_serial(limn_t *cpu, uint32_t address) {
    if (address == 4) {
        return cpu->serial.current_data & 0xFF;
    }
    if (address == 5) {
        return cpu->serial.current_data >> 8;
    }
    return 0;
}

uint8_t read_platboard(limn_t *cpu, uint32_t address) {
    return address % 2 == 0 ? 1 : 0;
}

uint8_t ignore_read(limn_t *cpu, uint32_t address) {
    return 0;
}
void ignore_write(limn_t *cpu, uint32_t address,uint8_t value) {}

limn_bus_section_t attachments[7] = {
    {
        .start = 0,
        .end = LIMN_RAM_SIZE,
        .read_byte = read_ram,
        .write_byte = write_ram
    },
    {
        .start = 0x10000000,
        .end = 0x10000007,
        .read_byte = read_ram_board,
        .write_byte = ignore_write
    },
    {
        .start = LIMN_SERIAL_CMD,
        .end = LIMN_SERIAL_CMD + 7,
        .read_byte = read_serial,
        .write_byte = write_serial
    },
    {
        .start = LIMN_PLATFORMBOARD,
        .end = LIMN_PLATFORMBOARD + 3,
        .read_byte = read_platboard,
        .write_byte = ignore_write
    },
    {
        .start = 0xC0000000,
        .end = 0xFFFDFFFF,
        .read_byte = ignore_read,
        .write_byte = ignore_write
    },
    {
		.start = 0x38000000,
		.end = 0x38000020,
		.read_byte = ignore_read,
		.write_byte = ignore_write
	},
    {
        .start = 0xFFFE0000,
        .end = 0xFFFFFFFF,
        .read_byte = read_rom,
        .write_byte = ignore_write
    },
};

void limn_reset(limn_t *cpu) {
    cpu->running = true;
    cpu->p_running = false;
    cpu->ticks = 0;
    cpu->running_time = 0;
    for (int i = 31; i < 42; i++) {
        cpu->registers[i] = 0;
    }
    if (cpu->call_stack) {
		sb_free(cpu->call_stack);
	}
    cpu->call_stack = NULL;
}

void limn_set_speed(limn_t *cpu, size_t speed) {
    cpu->speed = speed;
}

static int current_opcode;

uint8_t read_byte(limn_t *cpu, uint32_t address) {
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start);
        }
    }
    BUSERR;
    cpu->running = false;
    return -1;
}

uint16_t read_int(limn_t *cpu, uint32_t address) {
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 1 <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start) 
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 1) << 8);
        }
    }
    BUSERR;
    cpu->running = false;
    return -1;
}

uint32_t read_long(limn_t *cpu, uint32_t address) {
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 3 <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start) 
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 1) << 8)
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 2) << 16)
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 3) << 24);
        }
    }
    BUSERR;
    cpu->running = false;
    return -1;
}

void write_byte(limn_t *cpu, uint32_t address, uint8_t value) {
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value);
            return;
        }
    }
    BUSERR;
    cpu->running = false;
}

void write_int(limn_t *cpu, uint32_t address, uint16_t value) {
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 1 <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 1, value >> 8);
            return;
        }
    }
    BUSERR;
    cpu->running = false;
}

void write_long(limn_t *cpu, uint32_t address, uint32_t value) {
	if (address == 0xFFFFFFFF) {
		BUSERR;
		cpu->running = false;
		return;
	}
    for (size_t i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 3 <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 1, (value >> 8) & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 2, (value >> 16) & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 3, value >> 24);
            return;
        }
    }
    BUSERR;
    cpu->running = false;
}

uint8_t pc_byte(limn_t *cpu) {
    return read_byte(cpu, cpu->registers[LIMN_PC]++);
}

uint16_t pc_int(limn_t *cpu) {
    uint16_t s = read_int(cpu, cpu->registers[LIMN_PC]);
    cpu->registers[LIMN_PC] += 2;
    return s;
}

uint32_t pc_long(limn_t *cpu) {
    uint32_t l = read_long(cpu, cpu->registers[LIMN_PC]);
    cpu->registers[LIMN_PC] += 4;
    return l;
}

const char *SR(int r) {
    const char *names[42] = {
        "r0",
		"r1",
		"r2",
		"r3",
		"r4",
		"r5",
		"r6",
		"r7",
		"r8",
		"r9",
		"r10",
		"r11",
		"r12",
		"r13",
		"r14",
		"r15",
		"r16",
		"r17",
		"r18",
		"r19",
		"r20",
		"r21",
		"r22",
		"r23",
		"r24",
		"r25",
		"r26",
		"r27",
		"r28",
		"r29",
		"r30",
		"rf",

		"pc",
		"sp",
		"rs",
		"ivt",
		"htta",
		"usp",

		"k0",
		"k1",
		"k2",
		"k3"
    };
    return names[r];
}

void push(limn_t *cpu, int sp, uint32_t value) {
    R(sp) -= 4;
    write_long(cpu, R(sp), value);
}

void pop(limn_t *cpu, int sp, int r) {
    R(r) = read_long(cpu, R(sp));
    R(sp) += 4;
}

bool limn_cycle(limn_t *cpu) {
    bool thorough_debugger = false;
    if (thorough_debugger) {
        ALLOWED "At 0x%.8x, ", R(LIMN_PC));
    }
    current_opcode = pc_byte(cpu);
    switch (current_opcode) {
        case 0x00:
            ALLOWED "noping\n");
            break;
        case 0x01:{ ///LI
            uint8_t r = pc_byte(cpu);
            uint32_t value = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Setting register %s to 0x%.8x\n", SR(r), value);
                }
                //TODO : if (r == LIMN_RS && value & 0x80000000 != 0) reset bus
                cpu->registers[r] = value;
            }
            else {
                FORBIDDEN "Setting register %s to 0x%.8x\n", SR(r), value);
            }
            break;}
        case 0x02: { /// MOV
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Setting %s to %.8x@%s\n", SR(R0), R(R1), SR(R1));
                }
                R(R0) = R(R1);
            }
            else {
                FORBIDDEN "Setting %s to %.8x@%s\n", SR(R0), R(R1), SR(R1));
            }
            break;
        }
        case 0x03: {
            /// xch r0, r1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "Swapping %.8x@%s with %.8x@%s\n", R(R0), SR(R0), R(R1), SR(R1));
				}
                uint32_t t = R(R0);
                R(R0) = R(R1);
                R(R1) = t;
            }
            else {
                FORBIDDEN "Swapping %.8x@%s with %.8x@%s\n", R(R0), SR(R0), R(R1), SR(R1));
            }
            break;
        }
        case 0x04: {
            /// R0 = byte *IMM
            uint8_t r = pc_byte(cpu);
            uint32_t imm = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.2x@%.8x into %s\n", read_byte(cpu, imm), imm, SR(r));
                }
                R(r) = read_byte(cpu, imm);
            }
            else {
                FORBIDDEN "Storing %.2x@%.8x into %s\n", read_byte(cpu, imm), imm, SR(r));
            }
            break;
        }
        case 0x05: {
            /// R0 = int *IMM
            uint8_t r = pc_byte(cpu);
            uint32_t imm = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.4x@%.8x into %s\n", read_int(cpu, imm), imm, SR(r));
                }
                R(r) = read_int(cpu, imm);
            }
            else {
                FORBIDDEN "Storing %.2x@%.8x into %s\n", read_byte(cpu, imm), imm, SR(r));
            }
            break;
        }
        case 0x06:{ /// LRI.L R0, IMM
            uint8_t r = pc_byte(cpu);
            uint32_t addr = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Setting register %s to 0x%.8x@0x%.8x\n", SR(r), read_long(cpu, addr), addr);
                }
                R(r) = read_long(cpu, addr);
            }
            else {
                FORBIDDEN  "Setting register %s to 0x%.8x@0x%.8x\n", SR(r), read_long(cpu, addr), addr);
            }
            break;}
        case 0x07: {
            /// *IMM = byte R0
            uint32_t IMM = pc_long(cpu);
            uint8_t R0 = pc_byte(cpu);
            if (REG_ALLOWED(R0)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.2x@%s into %.8x\n", R(R0) & 0xFF, SR(R0), IMM);
                }
                write_byte(cpu, IMM, R(R0) & 0xFF);
            }
            else {
                FORBIDDEN "Storing %.2x@%s into %.8x\n", R(R0) & 0xFF, SR(R0), IMM);
            }
            break;
        }
        case 0x08: {
            /// *IMM = int R0
            uint32_t IMM = pc_long(cpu);
            uint8_t R0 = pc_byte(cpu);
            if (REG_ALLOWED(R0)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.4x@%s into %.8x\n", R(R0) & 0xFFFF, SR(R0), IMM);
                }
                write_int(cpu, IMM, R(R0) & 0xFFFF);
            }
            else {
                FORBIDDEN "Storing %.4x@%s into %.8x\n", R(R0) & 0xFFFF, SR(R0), IMM);
            }
            break;
        }
        case 0x09: {
            /// *IMM = long R0
            uint32_t IMM = pc_long(cpu);
            uint8_t R0 = pc_byte(cpu);
            if (REG_ALLOWED(R0)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.8x@%s into %.8x\n", R(R0), SR(R0), IMM);
                }
                write_long(cpu, IMM, R(R0));
            }
            else {
                FORBIDDEN "Storing %.8x@%s into %.8x\n", R(R0), SR(R0), IMM);
            }
            break;
        }
        case 0x0A: {
            /// R0 = byte *R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Storing 0x%.2x@%s into %s\n", read_byte(cpu, R(R1)), SR(R1), SR(R0));
                }
                R(R0) = read_byte(cpu, R(R1));
            }
            else {
                FORBIDDEN "Storing %.2x@%s into %s\n", read_byte(cpu, R1), SR(R1), SR(R0));
            }
            break;
        }
        case 0x0B: {
            /// R0 = int *R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Storing 0x%.4x@%s into %s\n", read_int(cpu, R(R1)), SR(R1), SR(R0));
                }
                R(R0) = read_int(cpu, R(R1));
            }
            else {
                FORBIDDEN "Storing 0x%.4x@%s into %s\n", read_int(cpu, R(R1)), SR(R1), SR(R0));
            }
            break;
        }
        case 0x0C: {
            /// R0 = long *R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Storing 0x%.8x@%s into %s\n", read_long(cpu, R(R1)), SR(R1), SR(R0));
                }
                R(R0) = read_long(cpu, R(R1));
            }
            else {
                FORBIDDEN "Storing 0x%.8x@%s into %s\n", read_long(cpu, R(R1)), SR(R1), SR(R0));
            }
            break;
        }
        case 0x0D: {
            // Formula: *R0 = byte R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "%.8x@%s = %.2x @ %s\n", R(R0), SR(R0), R(R1) & 0xFF, SR(R1));
				}
                write_byte(cpu, R(R0), R(R1) & 0xFF);
            }
            else {
				FORBIDDEN "%.8x@%s = %.2x @ %s\n", R(R0), SR(R0), R(R1) & 0xFF, SR(R1));
			}
            break;
        }
        case 0x0E: {
            // Formula: *R0 = int R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "%.8x@%s = %.4x @ %s\n", R(R0), SR(R0), R(R1) & 0xFFFF, SR(R1));
				}
                write_int(cpu, R(R0), R(R1) & 0xFFFF);
            }
            else {
				FORBIDDEN "%.8x@%s = %.4x @ %s\n", R(R0), SR(R0), R(R1) & 0xFFFF, SR(R1));
			}
            break;
        }
        case 0x0F: {
            // Formula: *R0 = long R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "%.8x@%s = %.8x @ %s\n", R(R0), SR(R0), R(R1), SR(R1));
				}
                write_long(cpu, R(R0), R(R1));
            }
            else {
				FORBIDDEN "%.8x@%s = %.8x @ %s\n", R(R0), SR(R0), R(R1), SR(R1));
			}
            break;
        }
        case 0x10: {
            /// *IMM0 = byte IMM1
            uint32_t IMM0 = pc_long(cpu);
            uint8_t IMM1 = pc_byte(cpu);
            if (thorough_debugger) {
                ALLOWED "Storing %.2x into %.8x\n", IMM1, IMM0);
            }
            write_byte(cpu, IMM0, IMM1);
            break;
        }
        case 0x11: {
            /// *IMM0 = int IMM1
            uint32_t IMM0 = pc_long(cpu);
            uint16_t IMM1 = pc_int(cpu);
            if (thorough_debugger) {
                ALLOWED "Storing %.4x into %.8x\n", IMM1, IMM0);
            }
            write_int(cpu, IMM0, IMM1);
            break;
        }
        case 0x12: {
            /// *IMM0 = long IMM1
            uint32_t IMM0 = pc_long(cpu);
            uint32_t IMM1 = pc_long(cpu);
            if (thorough_debugger) {
                ALLOWED "Storing %.8x into %.8x\n", IMM1, IMM0);
            }
            write_long(cpu, IMM0, IMM1);
            break;
        }
        case 0x13: {
            uint8_t r = pc_byte(cpu);
            uint8_t imm = pc_byte(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.2x to %.8x@%s\n", imm, R(r), SR(r));
                }
                write_byte(cpu, R(r), imm);
            }
            else {
                FORBIDDEN "Storing %.2x to %.8x@%s\n", imm, R(r), SR(r));
            }
            break;
        }
        case 0x14: {
            uint8_t r = pc_byte(cpu);
            uint16_t imm = pc_int(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.4x to %.8x@%s\n", imm, R(r), SR(r));
                }
                write_int(cpu, R(r), imm);
            }
            else {
                FORBIDDEN "Storing %.4x to %.8x@%s\n", imm, R(r), SR(r));
            }
            break;
        }
        case 0x15: {
            uint8_t r = pc_byte(cpu);
            uint32_t imm = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Storing %.8x to %.8x@%s\n", imm, R(r), SR(r));
                }
                write_long(cpu, R(r), imm);
            }
            else {
                FORBIDDEN "Storing %.8x to %.8x@%s\n", imm, R(r), SR(r));
            }
            break;
        }
        case 0x16: {
            uint8_t r = pc_byte(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Pushing %.8x@%s to the stack\n", R(r), SR(r));
                }
                push(cpu, CUR_STACK, R(r));
            }
            else {
                FORBIDDEN "Pushing %.8x to the stack\n", R(r));
            }
            break;
        }
        case 0x17: {
            uint32_t imm = pc_long(cpu);
            if (thorough_debugger) {
                ALLOWED "Pushing immediate %.8x to stack\n", imm);
            }
            push(cpu, CUR_STACK, imm);
            break;
        }
        case 0x18: {
            uint8_t r = pc_byte(cpu);
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "Popping %.8x into %s\n", read_long(cpu, R(CUR_STACK)), SR(r));
                }
                pop(cpu, CUR_STACK, r);
            }
            else {
                FORBIDDEN "Popping %.8x into %s\n", read_long(cpu, R(CUR_STACK)), SR(r));
            }
            break;
        }
        case 0x19: {
			/// pusha
			ALLOWED "Pushing all general purpose registers to the stack\n");
			for (int i = 0; i < 32; i++) {
				push(cpu, CUR_STACK, R(i));
			}
			break;
		}
        case 0x1A: {
			/// popa
			ALLOWED "Popping all general purpose registers from the stack\n");
			for (int i = 31; i >= 0; i--) {
				pop(cpu, CUR_STACK, i);
			}
			break;
		}
        case 0x1B:
            BRANCH(true, "True");
            break;
        case 0x1C: {
			uint8_t r = pc_byte(cpu);
			if (REG_ALLOWED(r)) {
				if (thorough_debugger) {
					ALLOWED "Branching to %.8x@%s\n", R(r), SR(r));
				}
				R(LIMN_PC) = R(r);
			}
			else {
				FORBIDDEN "Branching to %.8x@%s\n", R(r), SR(r));
			}
			break;
		}
        case 0x1D:
            BRANCH((R(LIMN_RF) & 0x01) == 0x01, "equal to");
            break;
        case 0x1E:
            BRANCH((R(LIMN_RF) & 0x01) != 0x01, "not equal to");
            break;
        case 0x1F:
			BRANCH((R(LIMN_RF) & 0x02) == 0x02, "greater than");
			break;
        case 0x20: {
            BRANCH((R(LIMN_RF) & 0x03) == 0, "less than");
            break;
        }
        case 0x21: { ///bge
            BRANCH((R(LIMN_RF) & 0x03) != 0, "greater than or equal to");
            break;
        }
        case 0x22: { 
            BRANCH((R(LIMN_RF) & 0x02) == 0, "less than or equal to");
            break;
        }
        case 0x23: {
            uint32_t imm = pc_long(cpu);
            if (thorough_debugger) {
                ALLOWED "Calling %.8x\n", imm);
            }
            CALL(imm);
            if (branches) {
				dump_backtrace(cpu);
			}
            break;
        }
        case 0x24: {
            pop(cpu, CUR_STACK, LIMN_PC);
            sb_pop(cpu->call_stack, 1);
            if (thorough_debugger) {
                ALLOWED "Returning to %.8x\n", R(LIMN_PC));
            }
            if (branches) {
				dump_backtrace(cpu);
			}
            break;
        }
        case 0x25: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            bool e = R(R0) == R(R1);
            bool g = R(R0) > R(R1);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "CMP: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
                }
                R(LIMN_RF) = (R(LIMN_RF) & 0xFFFFFFFC);
                if (e)
					R(LIMN_RF) |= 0x01;
				if (g)
					R(LIMN_RF) |= 0x02;
            }
            else {
                FORBIDDEN "CMP: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
            }
            break;
        }
        case 0x26: { 
            uint8_t r = pc_byte(cpu);
            uint32_t imm = pc_long(cpu);
            bool e = R(r) == imm;
            bool g = R(r) > imm;
            if (REG_ALLOWED(r)) {
                if (thorough_debugger) {
                    ALLOWED "CMPI: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
                }
                R(LIMN_RF) = (R(LIMN_RF) & 0xFFFFFFFC);
                if (e)
					R(LIMN_RF) |= 0x01;
				if (g)
					R(LIMN_RF) |= 0x02;
            }
            else {
                FORBIDDEN "CMPI: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
            }
            break;
        }
        case 0x27: {
            /// R0 = R1 + R2
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t R2 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
                if (thorough_debugger) {
					ALLOWED "%s = %.8x (%.8x@%s + %.8x@%s)\n", SR(R0), R(R1) + R(R2), R(R1), SR(R1), R(R2), SR(R2));
				}
                R(R0) = R(R1) + R(R2);
            }
            else {
				FORBIDDEN "%s = %.8x (%.8x@%s + %.8x@%s)\n", SR(R0), R(R0), R(R1), SR(R1), R(R2), SR(R2));
			}
            break;
        }
        case 0x28: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint32_t IMM = pc_long(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Adding %.8x and %.8x@%s and storing the result of %.8x in %s\n", IMM, R(R1), SR(R1), IMM + R(R1), SR(R0));
                }
                R(R0) = R(R1) + IMM;
            }
            else {
                FORBIDDEN "Adding %.8x and %.8x@%s and storing the result of %.8x in %s\n", IMM, R(R1), SR(R1), IMM + R(R1), SR(R0));
            }
            break;
        }
        case 0x29: {
            /// R0 = R1 - R2
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t R2 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
                R(R0) = R(R1) - R(R2);
				if (thorough_debugger) {
					ALLOWED "Subtracting %.8x@%s from %.8x@%s and storing the result of %.8x in %s\n", R(R1), SR(R1), R(R2), SR(R2), R(R1) - R(R2), SR(R0));
				}
            }
            else {
				FORBIDDEN "Subtracting %.8x@%s from %.8x@%s and storing the result of %.8x in %s\n", R(R1), SR(R1), R(R2), SR(R2), R(R1) - R(R2), SR(R0));
			}
            break;
        }
        case 0x2A: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint32_t IMM = pc_long(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
                    ALLOWED "Subtracting %.8x from %.8x@%s and storing the result of %.8x in %s\n", IMM, R(R1), SR(R1), R(R1) - IMM, SR(R0));
                }
                R(R0) = R(R1) - IMM;
            }
            else {
                FORBIDDEN "Subtracting %.8x from %.8x@%s and storing the result of %.8x in %s\n", IMM, R(R1), SR(R1), R(R1) - IMM, SR(R0));
            }
            break;
        }
        case 0x2B: {
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint8_t R2 = pc_byte(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
				if (thorough_debugger) {
					ALLOWED "Multiplying %.8x@%s and %.8x@%s into %s\n", R(R1), SR(R1), R(R2), SR(R2), SR(R0));
				}
				R(R0) = R(R2) * R(R1);
			}
			else {
				FORBIDDEN "Multiplying %.8x@%s and %.8x@%s into %s\n", R(R1), SR(R1), R(R2), SR(R2), SR(R0));
			}
			break;
		}
		case 0x2C: {
			// muli r0, r1, imm; r0 = r1 * imm
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint32_t imm = pc_long(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "Multiplying %.8x@%s and %.8x@IMM into %s\n", R(R1), SR(R1), imm, SR(R0));
				}
				R(R0) = imm * R(R1);
			}
			else {
				FORBIDDEN "Multiplying %.8x@%s and %.8x@IMM into %s\n", R(R1), SR(R1), imm, SR(R0));
			}
			break;
		}
		case 0x2D: {
			//~ R0 = floor(R1 / R2)
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint8_t R2 = pc_byte(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
				if (thorough_debugger) {
					ALLOWED "%s = floor(%.8x@%s / %.8x@%s) = %.8x", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) / R(R2));
				}
				R(R0) = R(R1) / R(R2);
			}
			else {
				FORBIDDEN "%s = floor(%.8x@%s / %.8x@%s) = %.8x", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) / R(R2));
			}
			break;
		}
		case 0x2E: {
			//~ R0 = floor(R1 / IMM)
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint32_t IMM = pc_long(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "%s = floor(%.8x@%s / %.8x) = %.8x", SR(R0), R(R1), SR(R1), IMM, R(R1) / IMM);
				}
				R(R0) = R(R1) / IMM;
			}
			else {
				FORBIDDEN "%s = floor(%.8x@%s / %.8x) = %.8x", SR(R0), R(R1), SR(R1), IMM, R(R1) / IMM);
			}
			break;
		}
		case 0x2F: {
			//~ R0 = R1 % R2
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint8_t R2 = pc_byte(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
				if (thorough_debugger) {
					ALLOWED "%s = %.8x@%s %% %.8x@%s = %.8x", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) % R(R2));
				}
				R(R0) = R(R1) % R(R2);
			}
			else {
				FORBIDDEN "%s = %.8x@%s %% %.8x@%s = %.8x", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) % R(R2));
			}
			break;
		}
		case 0x30: {
			//~ R0 = R1 % IMM
			uint8_t R0 = pc_byte(cpu);
			uint8_t R1 = pc_byte(cpu);
			uint32_t IMM = pc_long(cpu);
			if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "%s = %.8x@%s %% %.8x = %.8x", SR(R0), R(R1), SR(R1), IMM, R(R1) % IMM);
				}
				R(R0) = R(R1) % IMM;
			}
			else {
				FORBIDDEN "%s = %.8x@%s %% %.8x = %.8x", SR(R0), R(R1), SR(R1), IMM, R(R1) % IMM);
			}
			break;
		}
        case 0x31: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "Setting %s to %.8x, the inverse of %.8x@%s\n", SR(R0), ~R(R1), R(R1), SR(R1));
				}
                R(R0) = ~R(R1);
            }
            else {
				FORBIDDEN "Setting %s to %.8x, the inverse of %.8x@%s\n", SR(R0), ~R(R1), R(R1), SR(R1));
			}
            break;
        }
        case 0x32: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t R2 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
                R(R0) = R(R1) | R(R2);
                if (thorough_debugger) {
					ALLOWED "Setting %s to %.8x@%s | %.8x@%s = %.8x\n", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) | R(R2));
				}
            }
            else {
				FORBIDDEN "Setting %s to %.8x@%s | %.8x@%s = %.8x\n", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) | R(R2));
			}
            break;
        }
        case 0x38: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t R2 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1) && REG_ALLOWED(R2)) {
                R(R0) = R(R1) & R(R2);
                if (thorough_debugger) {
					ALLOWED "Setting %s to %.8x@%s & %.8x@%s = %.8x\n", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) & R(R2));
				}
            }
            else {
				FORBIDDEN "Setting %s to %.8x@%s & %.8x@%s = %.8x\n", SR(R0), R(R1), SR(R1), R(R2), SR(R2), R(R1) & R(R2));
			}
            break;
        }
        case 0x39: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint32_t IMM = pc_long(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                R(R0) = R(R1) & IMM;
                if (thorough_debugger) {
					ALLOWED "Setting %s to %.8x@%s & %.8x = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) & IMM);
				}
            }
            else {
				FORBIDDEN "Setting %s to %.8x@%s & %.8x = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) & IMM);
			}
            break;
        }
        case 0x41: {
			// BSETI R0, R1, byte IMM; R0 = R1 | 1 << IMM
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t IMM = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "%s = %s | 1 << %d\n", SR(R0), SR(R1), IMM);
				}
				R0 = R1 | (1 << IMM);
			}
			else {
				FORBIDDEN "%s = %s | 1 << %d\n", SR(R0), SR(R1), IMM);
			}
			break;
		}
        case 0x3F: {
            // Formula: R0 = R1 >> IMM
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t IMM = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "%s = %.8x@%s >> %.2x = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) >> IMM);
				}
                R(R0) = R(R1) >> IMM;
            }
            else {
				FORBIDDEN "%s = %.8x@%s >> %.2x = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) >> IMM);
			}
            break;
        }
        case 0x43: {
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            uint8_t IMM = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                if (thorough_debugger) {
					ALLOWED "%s = %.8x@%s ^ (1 << %.2x) = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) ^ (1 << IMM));
				}
                R(R0) = R(R1) ^ (1 << IMM);
            }
            else {
				FORBIDDEN "%s = %.8x@%s ^ (1 << %.2x) = %.8x\n", SR(R0), R(R1), SR(R1), IMM, R(R1) ^ (1 << IMM));
			}
            break;
        }
        case 0x45:
            LIMN_LOG(cpu, ZL_WARN, "CLI: Interrupts not yet implemented!\n");
            break;
        case 0x4C: {
            // TODO: log
            if (KERNEL_MODE) {
                R(0) = 0x80010000;
                if (thorough_debugger) {
					ALLOWED "R0 = CPUID\n");
				}
            }
            else {
				FORBIDDEN "R0 = CPUID\n");
			}
            break;
        }
        case 0x4F: {
            // pushv
            // R0 = R0 - 4; *R0 = long R1
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
				if (thorough_debugger) {
					ALLOWED "Pushing %.8x@%s to arbitrary stack %.8x@%s\n", R(R1), SR(R1), R(R0), SR(R0));
				}
                R(R0) -= 4;
                write_long(cpu, R(R0), R(R1));
            }
            else {
				FORBIDDEN "Pushing %.8x@%s to arbitrary stack %.8x@%s\n", R(R1), SR(R1), R(R0), SR(R0));
			}
            break;
        }
        case 0x50: {
            // R0 = R0 - 4; *R0 = long IMM
            uint8_t R0 = pc_byte(cpu);
            uint32_t IMM = pc_long(cpu);
            if (REG_ALLOWED(R0)) {
                R(R0) -= 4;
                write_long(cpu, R(R0), IMM);
            }
            break;
        }
        case 0x51: {
            // R1 = *R0; R0 = R0 + 4
            uint8_t R0 = pc_byte(cpu);
            uint8_t R1 = pc_byte(cpu);
            if (REG_ALLOWED(R0) && REG_ALLOWED(R1)) {
                R(R1) = read_long(cpu, R(R0));
				if (thorough_debugger) {
					ALLOWED "%s = %.8x@%.8x@%s; %s += 4\n", SR(R1), R(R1), R(R0), SR(R0), SR(R0));
				}
                R(R0) += 4;
            }
            else {
				FORBIDDEN "%s = %.8x@%s; %s += 4\n", SR(R1), R(R0), SR(R0), SR(R0));
			}
            break;
        }
        case 0xF1: {
			bool p_d = debuggerize;
			debuggerize = true;
            ALLOWED "%c", R(0));
            debuggerize = p_d;
			break;
		}
        default:
            LIMN_LOG(cpu,ZL_ERROR, "Unimplemented opcode: 0x%.2x / %c\n\tBreaking...\n", current_opcode, current_opcode);
            cpu->running = false;
            return false;
    }
    return cpu->running;
}

void limn_set_running(limn_t *cpu, bool running) {
    cpu->running = running;
}

void limn_destroy(limn_t *cpu) {
    limn_rom_destroy(cpu->rom);
    free(cpu->ram);
    free(cpu);
}

void limn_execute(limn_t *cpu) {
    if (cpu->running) {
        if (!cpu->p_running) {
            cpu->p_running = true;
            cpu->last_time = s_zany_elapsed();
        }
        double now = s_zany_elapsed();
        cpu->running_time += (now - cpu->last_time);
        size_t target_ticks = cpu->speed * cpu->running_time;
        if (target_ticks >= cpu->ticks) {
            size_t ticks = target_ticks - cpu->ticks;
            while (ticks-- > 0 && limn_cycle(cpu)) {
                cpu->ticks++;
            }
        }
        cpu->last_time = now;
    }
    else {
        cpu->p_running = false;
    }
}

void limn_set_serial(limn_t *cpu, void(*write)(uint32_t), uint32_t(*read)()) {
	cpu->serial.write = write;
    cpu->serial.read = read;
}

void limn_set_serial_read(limn_t *cpu, uint32_t(*read)()) {
    limn_set_serial(cpu, cpu->serial.write, read);
}

void limn_set_serial_write(limn_t *cpu, void(*write)(uint32_t)) {
    limn_set_serial(cpu, write, cpu->serial.read);
}

limn_t *limn_create(limn_rom_t *rom) {
    limn_t *l = NULL;
    if (rom != NULL) {
        l = malloc(sizeof(limn_t));
        l->speed = 1 Hz;
        l->rom = rom;
        l->ram = malloc(LIMN_RAM_SIZE);
        l->serial.read = NULL;
        l->serial.write = NULL;
        l->attachments = attachments;
        l->attachment_count = 7;
        l->call_stack = NULL;
        limn_reset(l);
        l->registers[LIMN_PC] = read_long(l, 0xFFFE0000);
        LIMN_LOG(l, ZL_DEBUG, "Initial address: %.8lx\n", l->registers[LIMN_PC]);
    }
    else {
        LIMN_LOG(NULL, ZL_ERROR, "Attempt to create a VM without a valid ROM!\n");
    }
    return l;
}
