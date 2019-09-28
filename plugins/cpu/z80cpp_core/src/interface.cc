#include "ZanyCore.h"
#include "MMU.h"
#include <Zany80/Plugin.h>
#include <Zany80/API/graphics.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

ZanyCore *core;
unsigned long speed = 1 MHz;

menu_t *menu;
window_t *game_window;
widget_t *fb;
const int fb_w = 256, fb_h = 128;
uint8_t buf[fb_w * fb_h * 4];

uint32_t RGB(uint8_t r, uint8_t g, uint8_t b) {
	return r | (g << 8) | (b << 16) | (0xFF << 24);
	//~ return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

uint32_t palette[16] = {
	RGB(20, 12, 28),
	RGB(68, 36, 52),
	RGB(48, 52, 109),
	RGB(78, 74, 78),
	RGB(133, 76, 48),
	RGB(52, 101, 36),
	RGB(208, 70, 72),
	RGB(117, 113, 97),
	RGB(89, 125, 206),
	RGB(210, 125, 44),
	RGB(133, 149, 161),
	RGB(109, 170, 44),
	RGB(210, 170, 153),
	RGB(109, 194, 202),
	RGB(218, 212, 94),
	RGB(222, 238, 214)
};

static uint8_t fill_color = 0, outline_color = 1;

void clear() {
	for (int i = 0; i < fb_w * fb_h * 4; i += 4) {
		memcpy(buf + i, &palette[fill_color], 4);
	}
}

void set_pixel(uint8_t x, uint8_t y) {
	((uint32_t*)buf)[y * 256 + x] = palette[outline_color];
}

void line(int x1, int y1, int x2, int y2) {
	bool steep = false;
	if (abs(y1 - y2) > abs(x1 - x2)) {
		steep = true;
		std::swap(x1, y1);
		std::swap(x2, y2);
	}
	if (x1 > x2) {
		std::swap(x1, x2);
		std::swap(y1, y2);
	}
	// x1 = 20, y1 = 50, x2 = 30, y2 = 50
	for (int x = x1; x <= x2; x++) {
		float t = (x - x1) / (float)(x2 - x1);
		float y = y1 * (1. - t) + y2 * t;
		set_pixel(steep ? y : x, steep ? x : y);
	}
}

void tri(int x1, int y1, int x2, int y2, int x3, int y3) {
	line(x1, y1, x2, y2);
	line(x1, y1, x3, y3);
	line(x3, y3, x2, y2);
	
}

void mini_gpu(uint16_t address, uint8_t value) {
	// 0: command register, 1-2: fill and outline colors, 3-31: data
	static uint8_t registers[29];
	if (address == 0) {
		// execute command based on registers
		switch (value) {
			case 0:
				clear();
				break;
			case 1:
				set_pixel(registers[0], registers[1]);
				break;
			case 2:
				line(registers[0], registers[1], registers[2], registers[3]);
				break;
			case 3:
				tri(registers[0], registers[1], registers[2], registers[3], registers[4], registers[5]);
				break;
			default:
				puts("[Z81] Unknown command received.");
				break;
		}
	}
	else if (address == 1) {
		fill_color = value;
	}
	else if (address == 2) {
		outline_color = value;
	}
	else if (address < 32) {
		registers[address - 3] = value;
	}
}

void frame(float delta) {
	float x,y;
	render_window(game_window);
	window_get_size(game_window, &x, &y);
	image_set_size(fb, x, y - 35);
	if (mmu->isReady() != 1) {
		for (int i = 0; i < fb_w * fb_h; i++) {
			((uint32_t*)buf)[i] = rand();
		}
		return;
	}
	// Fire VSYNC
	core->fireInterrupt(0);
	core->execute(delta * speed);
}

bool supports(const char *type) {
	#define IS(x) (!strcmp(type, x))
	return IS("CPU") || IS("z80") || IS("Perpetual") || IS("z80cpp_core");
}

perpetual_plugin_t perpetual;
cpu_plugin_t cpu;
plugin_t plugin;

void unload() {
	mmu->unload();
}

extern "C" {

	PLUGIN_EXPORT void init() {
		core = new ZanyCore();
		mmu = new MMU(core);
		memset(&cpu, 0, sizeof(cpu_plugin_t));
		cpu.get_registers = []() -> list_t* {
			return core->get_registers();
		};
		cpu.get_cycle_count = []() -> uint64_t {
			return core->executedCycles();
		};
		cpu.execute = [](uint32_t cycles) {
			core->execute(cycles);
		};
		cpu.attach_output = [](uint32_t port, write_handler_t handler) {
			core->attachToPort(port, handler);
		};
		cpu.attach_input = [](uint32_t port, read_handler_t handler) {
			core->attachToPort(port, handler);
		};
		cpu.reset = []() {
			core->reset();
		};
		cpu.fire_interrupt = [](uint8_t irq) {
			core->fireInterrupt(irq);
		};
		cpu.load_rom = [](const char *rom) -> bool {
			return core->loadROM(rom);
		};
		memset(&perpetual, 0, sizeof(perpetual_plugin_t));
		perpetual.frame = frame;
		memset(&plugin, 0, sizeof(plugin_t));
		plugin.name = "z80cpp core";
		plugin.supports = supports;
		plugin.perpetual = &perpetual;
		plugin.cpu = &cpu;
		append_main_menu(menu = menu_create("CPU"));
		menu_append(menu, menuitem_create("Reset", cpu.reset));
		menu_append(menu, menuitem_create("Unload", unload));
		game_window = window_create("Zany80 Screen");
		//~ window_auto_size(game_window, true);
		fb = framebuffer_create(buf, fb_w, fb_h);
		image_set_size(fb, fb_w * 2.5, fb_h * 2.5);
		window_append(game_window, fb);
		for (int i = 0; i < fb_w * fb_h; i++) {
			buf[i * 4 + 3] = 255;
		}
	}
	
	PLUGIN_EXPORT plugin_t *get_interface() {
		return &plugin;
	}
	
	PLUGIN_EXPORT void cleanup() {
		delete mmu;
		delete core;
		mmu = nullptr;
		core = nullptr;
		window_remove_menu(get_root(), menu);
		menu_destroy_all(menu);
		menu_destroy(menu);
		window_destroy(game_window);
		widget_destroy(fb);
	}

}
