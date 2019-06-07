#pragma once

#include "limn.h"

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t limn_register_t;

#define LIMN_R0 0
#define LIMN_RF 31
#define LIMN_PC 32
#define LIMN_SP 33
#define LIMN_RS 34
#define LIMN_IVT 35
#define LIMN_HTTA 36
#define LIMN_USP 37
#define LIMN_K0 38

#define LIMN_SERIAL_CMD 0xF8000040
#define LIMN_SERIAL_DATA 0xF8000044
#define LIMN_SERIAL_WRITE 1
#define LIMN_SERIAL_READ 2

#define LIMN_RAM_SIZE 1024 * 1024
#define LIMN_RAM_SLOTCOUNT 0x1000000

#define LIMN_PLATFORMBOARD 0xF8000800

struct limn_rom_t {
    char *path;
    char *buf;
    size_t rom_size;
};

typedef struct {
    uint16_t current_data;
    void (*write)(uint32_t data);
    uint32_t (*read)();
} limn_serial_t;

typedef struct {
    void (*write_byte)(limn_t *cpu, uint32_t address, uint8_t data);
    uint8_t(*read_byte)(limn_t *cpu, uint32_t address);
    size_t start, end;
} limn_bus_section_t;

struct limn {
    limn_rom_t *rom;
    limn_register_t registers[42];
    limn_serial_t serial;
    limn_bus_section_t *attachments;
    size_t attachment_count;
    size_t speed;
    size_t ticks;
    double running_time, last_time;
    bool running, p_running;
    char *ram;
};

const char *SR(int r);