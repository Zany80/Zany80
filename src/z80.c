#include "z80.h"
#include "global.h"
#include "serial.h"
#include "zexall.h"

#include "z80e/ti/asic.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MHz * 1000 * 1000
const size_t speed = 50 MHz;

static asic_t *asic = NULL;
// callback_widget is NULL when z80 is halted.
static widget_t *run, *callback_widget;

uint64_t executed;

static void zex_reset(void *device, uint8_t data) {
	(void)data;
	if (device != asic) {
		__builtin_unreachable();
	}
	const char msg[] = "Tests completed.\n";
	serial_write_all(msg, sizeof(msg) - 1);
	z80_halt();
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

void z80_halt() {
	asic->cpu->halted = true;
	executed = 0;
	if (callback_widget) {
		window_remove(get_root(), callback_widget);
		widget_destroy(callback_widget);
		callback_widget = NULL;
	}
}

static void z80_callback() {
	if (asic->cpu->halted) {
		executed = 0;
		return;
	}
	const size_t frame = speed / 60;
	cpu_execute(asic->cpu, frame);
	executed += frame;
}

void z80_unhalt() {
	if (!callback_widget) {
		asic->cpu->halted = false;
		callback_widget = customwidget_create(z80_callback);
		window_append(get_root(), callback_widget);
	}
}

static void z80_sout(void *device, uint8_t val) {
	if (device != asic) {
		__builtin_unreachable();
	}
	serial_write(val);
}

static uint8_t z80_sin(void *device) {
	if (device != asic) {
		__builtin_unreachable();
	}
	return serial_read();
}

void z80_jump(uint16_t target) {
	if (target == 0) {
		// TODO: don't use this hack
		asic->cpu->devices[0].write_out = z80_sout;
		asic->cpu->devices[0].read_in = z80_sin;
	}
	asic->cpu->registers.PC = target;
}

static void z80_run() {
	if (!callback_widget) {
		for (uint16_t i = 0; i < 0x100; i += 1) {
			asic->cpu->devices[i].write_out = zex_reset;
			asic->cpu->devices[i].read_in = write_text;
			asic->cpu->devices[i].device = asic;
		}
		memcpy(asic->mmu->ram + 0x100, zexall_com, zexall_com_len);
		asic->mmu->ram[0] = 0xd3; /* OUT N, A */
		asic->mmu->ram[1] = 0x00;

		asic->mmu->ram[5] = 0xdb; /* IN A, N */
		asic->mmu->ram[6] = 0x00;
		asic->mmu->ram[7] = 0xc9; /* RET */
		z80_jump(0x100);
		z80_unhalt();
	}
	else {
		const char msg[] = "Warning: z80 already running!\n";
		serial_write_all(msg, sizeof(msg) - 1);
	}
}

void z80_load(uint8_t *buf, size_t len) {
	if (len > 0x8000) {
		const char msg[] = "Warning: not loading, exceeded size.\n";
		serial_write_all(msg, sizeof(msg) - 1);
		return;
	}
	memcpy(asic->mmu->ram, buf, len);
}

void z80_init() {
	executed = 0;
	if (asic) {
		// TODO
	}
	asic = calloc(1, sizeof(asic_t));
	asic->log = NULL;
	asic->cpu = cpu_init(NULL);
	asic->mmu = ti_mmu_init(TI84p, NULL);
	asic->cpu->memory = (void*)asic->mmu;
	asic->cpu->read_byte = ti_read_byte;
	asic->cpu->write_byte = ti_write_byte;
	asic->battery = BATTERIES_GOOD;
	asic->device = TI84p;

	asic->timers = calloc(1, sizeof(z80_hardware_timers_t));
	asic->timers->max_timers = 20;
	asic->timers->timers = calloc(20, sizeof(z80_hardware_timer_t));

	asic->stopped = 0;
	asic->debugger = 0;
	asic->runloop = runloop_init(asic);

	asic->link = calloc(1, sizeof(z80_link_socket_t));
	asic_set_clock_rate(asic, speed);

	asic->mmu->banks[0].page = 0;
	asic->mmu->banks[1].page = 1;
	asic->mmu->banks[2].page = 2;
	asic->mmu->banks[3].page = 3;

	asic->mmu->banks[0].flash = 0;
	asic->mmu->banks[1].flash = 0;
	asic->mmu->banks[2].flash = 0;
	asic->mmu->banks[3].flash = 0;

	run = menuitem_create("Run zex tests", z80_run);
	menu_append(global_menu, run);
}

void z80_deinit() {
	if (asic) {
		free(asic->timers->timers);
		free(asic->timers);
		ti_mmu_free(asic->mmu);
		cpu_free(asic->cpu);
		free(asic);
		asic = NULL;
	}
}
