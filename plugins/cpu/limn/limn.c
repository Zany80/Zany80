#include "internals.h"
#include <stdlib.h>
#include <Zany80/API.h>
#include <stdbool.h>

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

limn_bus_section_t attachments[2] = {
    {
        .start = 0x10000000,
        .end = 0x10000007,
        .read_byte = read_ram_board
    },
    {
        .start = 0xFFFE0000,
        .end = 0xFFFFFFFF,
        .read_byte = read_rom
    }
};

void limn_reset(limn_t *cpu) {
    cpu->running = true;
    cpu->p_running = false;
    cpu->ticks = 0;
    cpu->running_time = 0;
    for (int i = 31; i < 42; i++) {
        cpu->registers[i] = 0;
    }
}

void serial_handler(limn_t *cpu, uint8_t command) {
    switch (command) {
        case LIMN_SERIAL_WRITE:
            if (cpu->serial.write) {
                cpu->serial.write(cpu->serial.current_data);
            }
            else {
                zany_log(ZL_DEBUG, "%c", cpu->serial.current_data);
            }
            break;
        case LIMN_SERIAL_READ:
            uint32_t value = 0xFFFF;
            if (cpu->serial.read) {
                value = cpu->serial.read();
            }
            cpu->serial.current_data = value & 0xFF;
            break;
        default:
            zany_log(ZL_ERROR, "Serial controller received unknown command %d\n", command);
            break;
    }
}

uint8_t read_byte(limn_t *cpu, uint32_t address) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start);
        }
    }
    return -1;
}

uint16_t read_int(limn_t *cpu, uint32_t address) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 1 <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start) 
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 1) << 8);
        }
    }
    return -1;
}

uint32_t read_long(limn_t *cpu, uint32_t address) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 3 <= cpu->attachments[i].end) {
            return cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start) 
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 1) << 8)
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 2) << 16)
            | (cpu->attachments[i].read_byte(cpu, address - cpu->attachments[i].start + 3) << 24);
        }
    }
    return -1;
}

void write_byte(limn_t *cpu, uint32_t address, uint8_t value) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value);
            break;
        }
    }
}

void write_int(limn_t *cpu, uint32_t address, uint16_t value) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 1 <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 1, value >> 8);
            break;
        }
    }
}

void write_long(limn_t *cpu, uint32_t address, uint32_t value) {
    for (int i = 0; i < cpu->attachment_count; i++) {
        if (address >= cpu->attachments[i].start && address + 3 <= cpu->attachments[i].end) {
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start, value & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 1, (value >> 8) & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 2, (value >> 16) & 0xFF);
            cpu->attachments[i].write_byte(cpu, address - cpu->attachments[i].start + 3, value >> 24);
            break;
        }
    }
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
    static char buf[32];
    if (r < 31) {
        sprintf(buf, "R%d", r);
    }
    else {
        r -= 31;
        static const char *names[11] = {
            "rf", "pc", "sp", "rs", "ivt", "htta", "usp", "k0", "k1", "k2", "k3"
        };
        strcpy(buf, names[r]);
    }
    return buf;
}

#define R(r) cpu->registers[r]
#define KERNEL_MODE ((R(LIMN_RS) & 0x01) == 0)
#define USER_MODE ((R(LIMN_RS) & 0x01) == 1)
#define REG_ALLOWED(r) (r < 32 || KERNEL_MODE)
#define FORBIDDEN zany_log(ZL_ERROR, "Access denied! Attempted: "
#define CUR_STACK (KERNEL_MODE ? LIMN_SP : LIMN_USP)

void push(limn_t *cpu, int sp, uint32_t value) {
    R(sp) -= 4;
    write_long(cpu, R(sp), value);
}

void limn_cycle(limn_t *cpu) {
    static bool debuggerize = true;
    uint8_t i = pc_byte(cpu);
    switch (i) {
        case 1:{ ///LI
            uint8_t r = pc_byte(cpu);
            uint32_t value = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (debuggerize) {
                    zany_log(ZL_DEBUG, "Setting register %s to 0x%.8x\n", SR(r), value);
                }
                //TODO : if (r == LIMN_RS && value & 0x80000000 != 0) reset bus
                cpu->registers[r] = value;
            }
            else {
                FORBIDDEN "Setting register %s to 0x%.8x\n", SR(r), value);
            }
            break;}
        case 0x06:{ /// LRI.L R0, IMM
            uint8_t r = pc_byte(cpu);
            uint32_t addr = pc_long(cpu);
            if (REG_ALLOWED(r)) {
                if (debuggerize) {
                    zany_log(ZL_DEBUG, "Setting register %s to 0x%.8x@0x%.8x\n", SR(r), read_long(cpu, addr), addr);
                }
            }
            else {
                FORBIDDEN  "Setting register %s to 0x%.8x@0x%.8x\n", SR(r), read_long(cpu, addr), addr);
            }
            break;}
        case 0x16: {
            uint8_t r = pc_byte(cpu);
            if (REG_ALLOWED(r)) {
                if (debuggerize) {
                    zany_log(ZL_DEBUG, "Pushing %.8x to the stack\n", R(r));
                }
                push(cpu, CUR_STACK, R(r));
            }
            else {
                FORBIDDEN "Pushing %.8x to the stack\n", R(r));
            }
        }
        case 0x20: {
            uint32_t imm = pc_long(cpu);
            if (R(LIMN_RF) & 0x03 == 0) {
                if (debuggerize) {
                    zany_log(ZL_DEBUG, "Branching to %.8x; condition: less than\n", imm);
                }
                R(LIMN_PC) = imm;
            }
            else {
                if (debuggerize) {
                    zany_log(ZL_DEBUG, "Not branching to %.8x; condition: less than\n", imm);
                }
            }
            break;
        }
        case 0x26: { 
            uint8_t r = pc_byte(cpu);
            uint32_t imm = pc_long(cpu);
            bool e = R(r) == imm;
            bool g = R(r) > imm;
            if (REG_ALLOWED(r)) {
                zany_log(ZL_DEBUG, "CMPI: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
                R(r) = (R(r) & 0xFFFFFFFC) | e ? 0x01 : 0x00 | g ? 0x02 : 0x00;
            }
            else {
                FORBIDDEN "CMPI: %s, %s\n", e ? "equal" : "not equal", g? "greater" : "not greater");
            }
            break;
        }
        case 0x45:
            zany_log(ZL_WARN, "CLI: Interrupts not yet implemented!\n");
            break;
        default:
            zany_log(ZL_ERROR, "Unimplemented opcode: 0x%.2x / %c\n", i, i);
            break;
    }
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
            while (ticks-- > 0) {
                cpu->ticks++;
                limn_cycle(cpu);
            }
        }
        cpu->last_time = now;
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
        l->attachment_count = 2;
        limn_reset(l);
        l->registers[LIMN_PC] = read_long(l, 0xFFFE0000);
        zany_log(ZL_DEBUG, "Initial address: %.8lx\n", l->registers[LIMN_PC]);
    }
    else {
        zany_log(ZL_ERROR, "Attempt to create a VM without a valid ROM!\n");
    }
    return l;
}
