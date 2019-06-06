#include "limn.h"
#include <stdbool.h>
#include <Zany80/Plugin.h>

limn_t *cpu;

void frame(float delta) {
    limn_execute(cpu);
}

bool supports(const char *type) {
	#define IS(x) (!strcmp(type, x))
	return IS("CPU") || IS("LIMN") || IS("Perpetual");
}

void execute(uint32_t cycles) {
    for (int i = 0; i < cycles; i++) {
        limn_cycle(cpu);
    }
}

perpetual_plugin_t perpetual = {
    .frame = frame
};

void attach_input(uint32_t port, read_handler_t handler) {
    if (port == 1) {
        limn_set_serial_read(cpu, (uint32_t(*)())handler);
    }
}

void attach_output(uint32_t port, write_handler_t handler) {
    if (port == 1) {
        limn_set_serial_write(cpu, (void(*)(uint32_t))handler);
    }
}

cpu_plugin_t cpu_interface = {
    .execute = execute,
    .attach_input = attach_input
};

plugin_t plugin = {
    .perpetual = &perpetual,
    .cpu = &cpu_interface,
    .supports = supports,
    .name = "LIMN1k CPU"
};

PLUGIN_EXPORT void init() {
    cpu = limn_create(limn_rom_load("data/antecedent.rom"));
}

PLUGIN_EXPORT plugin_t *get_interface() {
    return &plugin;
}

PLUGIN_EXPORT void cleanup() {
    limn_destroy(cpu);
}