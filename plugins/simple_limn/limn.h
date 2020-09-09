#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "SIMPLE/data.h"

typedef struct limn limn_t;
typedef struct limn_rom_t limn_rom_t;

#define Hz * 1
#define KHz * 1000 Hz
#define MHz * 1000 KHz

limn_rom_t *limn_rom_load(char *path);
limn_rom_t *limn_rom_load_simple(SIMPLEFile f);
void limn_rom_destroy(limn_rom_t *rom);

limn_t *limn_create(limn_rom_t *rom);
void limn_destroy(limn_t *);
void limn_reset(limn_t *);
bool limn_cycle(limn_t *);
void limn_execute(limn_t *);
void limn_set_speed(limn_t *, size_t);
void limn_set_running(limn_t *, bool);
/// write is called when the LIMM writes to the serial port, read is called
/// when the LIMN1k reads from the serial port
void limn_set_serial(limn_t *, void(*write)(uint32_t), uint32_t(*read)());
void limn_set_serial_read(limn_t *, uint32_t(*read)());
void limn_set_serial_write(limn_t *, void(*write)(uint32_t));
