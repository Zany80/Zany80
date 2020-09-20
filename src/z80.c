#include "z80.h"
#include "global.h"
#include "serial.h"
#include "zexall.h"

#include "z80e/ti/asic.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MHz * 1000 * 1000
const size_t speed = 500 MHz;

static asic_t *asic = NULL;
static widget_t *run, *callback_widget;

static bool stop = false;
uint64_t executed;

static void cpu_reset(void *device, uint8_t data) {
	(void)data;
	const asic_t *asic = device;
	const char msg[] = "Tests completed.\n";
	serial_write_all(msg, sizeof(msg) - 1);
	stop = true;
	asic->cpu->halted = true;
	window_remove(get_root(), callback_widget);
}

static uint8_t write_text(void *device) {
	asic_t *asic = device;

	if (asic->cpu->registers.C == 2) {
		serial_write(asic->cpu->registers.E);
	} else if (asic->cpu->registers.C == 9) {
		for (uint16_t i = asic->cpu->registers.DE; asic->mmu->ram[i & 0xffff] != '$'; i += 1) {
			serial_write(asic->mmu->ram[i & 0xffff]);
			if (asic->mmu->ram[i & 0xffff] == 0) {
				break;
			}
		}
	}
	return 0;
}

static void z80_callback() {
	const size_t frame = speed / 60;
	cpu_execute(asic->cpu, frame);
	executed += frame;
}

static void z80_run() {
	callback_widget = customwidget_create(z80_callback);
	window_append(get_root(), callback_widget);
}

void z80_init() {
	stop = false;
	executed = 0;
	if (asic) {
		// TODO
	}
	asic = asic_init(TI84p, NULL);
	asic_set_clock_rate(asic, speed);
	for (uint16_t i = 0; i < 0x100; i += 1) {
		asic->cpu->devices[i].write_out = cpu_reset;
		asic->cpu->devices[i].read_in = write_text;
		asic->cpu->devices[i].device = asic;
	}
	memcpy(asic->mmu->ram + 0x100, zexall_com, zexall_com_len);
	asic->mmu->ram[0] = 0xd3; /* OUT N, A */
	asic->mmu->ram[1] = 0x00;

	asic->mmu->ram[5] = 0xdb; /* IN A, N */
	asic->mmu->ram[6] = 0x00;
	asic->mmu->ram[7] = 0xc9; /* RET */	
	asic->mmu->banks[0].page = 0;
	asic->mmu->banks[1].page = 1;
	asic->mmu->banks[2].page = 2;
	asic->mmu->banks[3].page = 3;

	asic->mmu->banks[0].flash = 0;
	asic->mmu->banks[1].flash = 0;
	asic->mmu->banks[2].flash = 0;
	asic->mmu->banks[3].flash = 0;

	asic->cpu->registers.PC = 0x100;

	run = menuitem_create("Run zex tests", z80_run);
	menu_append(global_menu, run);
}

void z80_deinit() {
	if (asic) {
		ti_mmu_free(asic->mmu);
		cpu_free(asic->cpu);
		free(asic);
		asic = NULL;
	}
}
