#include "internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SIMPLE/API.h"

limn_rom_t *limn_rom_load(char *path) {
    limn_rom_t *rom = NULL;
    FILE *f = fopen(path, "rb");
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size > 0) {
            if (size > 0x20000) {
				simple_log(SL_ERROR, "ROM '%s' too big!\n");
			}
			else {
				rom = malloc(sizeof(limn_rom_t));
				rom->rom_size = size;
				rom->buf = malloc(128 * 1024);
				rewind(f);
				fread(rom->buf, 1, rom->rom_size, f);
				rom->buf -= 0x7FE0000;
				simple_log(SL_INFO, "ROM '%s' loaded successfully.\n", path);
			}
        }
        else {
            simple_log(SL_ERROR, "Error loading ROM '%s': file is empty!\n", path);
        }
        fclose(f);
    }
    else {
        simple_log(SL_ERROR, "Error loading file: %s\n", path);
    }
    return rom;
}

limn_rom_t *limn_rom_load_simple(SIMPLEFile f) {
	if (!f.valid || f.length > 0x20000) {
		return NULL;
	}
	limn_rom_t *rom = malloc(sizeof(limn_rom_t));
	assert(rom != NULL);
	rom->rom_size = f.length;
	rom->buf = malloc(128 * 1024);
	assert(rom->buf != NULL);
	memcpy(rom->buf, f.buf, f.length);
	rom->buf -= 0x7FE0000;
	return rom;
}

void limn_rom_destroy(limn_rom_t *rom) {
    if (rom) {
        rom->buf += 0x7FE0000;
        free(rom->buf);
        free(rom);
    }
}
