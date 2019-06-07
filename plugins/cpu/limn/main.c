#include "limn.h"
#include <stdbool.h>
#include <Zany80/Plugin.h>
#include <Zany80/API/graphics.h>

limn_t *cpu;
menu_t *menu;
size_t speed;
widget_t *label;
bool running;

void frame(float delta) {
    static char buf[64];
    sprintf(buf, "%s, current speed: %lluHz", running ? "Running" : "Paused",speed);
    widget_set_label(label, buf);
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
    speed = 1000000;
    limn_set_speed(cpu, 1000000);
}

PLUGIN_EXPORT void init() {
    cpu = limn_create(limn_rom_load("data/antecedent.rom"));
    limn_set_running(cpu, running = true);
    limn_set_speed(cpu, speed = 1000000);
    window_append_menu(get_root(), menu = menu_create("LIMN1k"));
    menu_append(menu, label = label_create(""));
    menu_append(menu, button_create("Decrease", slow_down));
    menu_append(menu, button_create("Increase", speed_up));
    menu_append(menu, button_create("Straight to max", max_speed));
    menu_append(menu, button_create("Play/Pause", toggle_running));
}

PLUGIN_EXPORT plugin_t *get_interface() {
    return &plugin;
}

PLUGIN_EXPORT void cleanup() {
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