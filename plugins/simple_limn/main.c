#include "limn.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "SIMPLE/Plugin.h"
#include "SIMPLE/data.h"
#include "SIMPLE/graphics.h"

plugin_version_t version = {
	.major = 1,
	.minor = 0,
	.patch = 5,
	.str = "1.0.5-alpha"
};


limn_t *cpu;
menu_t *menu;
size_t speed;
widget_t *label;
bool running;

void frame(float delta) {
    static char buf[64];
    sprintf(buf, "%s, current speed: %luHz", running ? "Running" : "Paused",speed);
    widget_set_label(label, buf);
    limn_execute(cpu);
}

bool supports(const char *type) {
	#define IS(x) (!strcmp(type, x))
	return IS("CPU") || IS("LIMN") || IS("Perpetual");
}

void execute(uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; i++) {
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

list_t *list_registers();

cpu_plugin_t cpu_interface = {
    .execute = execute,
    .attach_input = attach_input,
    .attach_output = attach_output,
    .get_registers = list_registers
};

plugin_t plugin = {
    .perpetual = &perpetual,
    .cpu = &cpu_interface,
    .supports = supports,
    .version = &version,
    .name = "LIMN1k CPU"
};

void slow_down() {
    if (speed < 2)
        return;
    speed -= 2;
    limn_set_speed(cpu, speed);
}

void speed_up() {
    if (speed >= 1000000)
        return;
    speed += 5;
    limn_set_speed(cpu, speed);
}

void toggle_running() {
    running = !running;
    limn_set_running(cpu, running);
}

void max_speed() {
    speed = 100000;
    limn_set_speed(cpu, 100000);
}

void reset() {
	limn_reset(cpu);
}

PLUGIN_EXPORT void init() {
	SIMPLEFile f = simple_data_file("Antecedent ROM", NULL);
    cpu = limn_create(limn_rom_load_simple(f));
    simple_data_free(f);
    if (cpu == NULL) {
        return;
    }
    limn_set_running(cpu, running = true);
    limn_set_speed(cpu, speed = 100 KHz);
    window_append_menu(get_root(), menu = menu_create("LIMN1k"));
    menu_append(menu, label = label_create(""));
    menu_append(menu, menuitem_create("Decrease", slow_down));
    menu_append(menu, menuitem_create("Increase", speed_up));
    menu_append(menu, menuitem_create("Straight to max", max_speed));
    menu_append(menu, menuitem_create("Play/Pause", toggle_running));
    menu_append(menu, menuitem_create("Reset", reset));
}

PLUGIN_EXPORT plugin_t *get_interface() {
    if (cpu == NULL) {
        return NULL;
    }
    return &plugin;
}

PLUGIN_EXPORT void cleanup() {
    if (cpu == NULL)
        return;
    limn_destroy(cpu);
    window_remove_menu(get_root(), menu);
    menu_destroy_all(menu);
    menu_destroy(menu);
}

#include "internals.h"

list_t *list_registers() {
    static register_value_t registers[42];
	list_t *regs = create_list();
	register_value_t *cur;
    for (int i = 0; i < 42; i++) {
		cur = &registers[i];
		cur->name = SR(i);
		cur->value = cpu->registers[i];
		list_add(regs, cur);
    }
	return regs;
}
