#include "internals.h"
#include <stdio.h>
#include <Zany80/API.h>
#include <stdlib.h>
#include <string.h>

limn_rom_t *limn_rom_load(char *path) {
    limn_rom_t *rom = NULL;
    FILE *f = fopen(path, "rb");
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size > 0) {
            if (size > 0x20000) {
				zany_log(ZL_ERROR, "ROM '%s' too big!\n");
			}
			else {
				rom = malloc(sizeof(limn_rom_t));
				rom->rom_size = size;
				rom->buf = malloc(128 * 1024);
				rom->path = strdup(path);
				rewind(f);
				fread(rom->buf, 1, rom->rom_size, f);
				rom->buf[rom->rom_size] = 0;
				rom->buf -= 0x7FE0000;
				zany_log(ZL_INFO, "ROM '%s' loaded successfully.\n", path);
			}
        }
        else {
            zany_log(ZL_ERROR, "Error loading ROM '%s': file is empty!\n", path);
        }
        fclose(f);
    }
    else {
        zany_log(ZL_ERROR, "Error loading file: %s\n", path);
    }
    return rom;
}

void limn_rom_destroy(limn_rom_t *rom) {
    if (rom) {
        zany_log(ZL_INFO, "Cleaning up ROM '%s'...\n", rom->path);
        free(rom->path);
        rom->buf += 0x7FE0000;
        free(rom->buf);
        free(rom);
    }
}
