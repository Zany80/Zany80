#include <Zany80/Plugin.h>
#include <Zany80/API/graphics.h>
#include <string.h>
#include <Zany80/3rd-party/scas/stringop.h>
#include <Zany80/ring_buffer.h>
#include <stdlib.h>
#include <stdio.h>

bool connected;
plugin_t *cpu;

plugin_t disabled = {
	.name = "None (Disabled)"
};

window_t *window;
menu_t *menu, *select_cpu;
widget_t *clear_button;
widget_t *output, *input;
widget_t *cpus;
list_t *cpu_list;

int current_cpu;

ring_buffer_t *input_buf;
char *output_buf;
size_t out_size;

uint32_t read_handler() {
	return ring_buffer_read(input_buf);
}

void clear() {
	out_size = 0;
	output_buf[0] = 0;
}

void output_handler(uint32_t value) {
	if (value == 0) {
		return;
	}
	if (out_size >= 1024 * 1024 * 32) {
		clear();
	}
	output_buf[out_size++] = (char)(value & 0xFF);
	output_buf[out_size] = 0;
}

bool cpu_handler(int index) {
	if (index >= cpu_list->length) {
		return false;
	}
	cpu = (plugin_t*)(cpu_list->items[index]);
	if (cpu->cpu && cpu->cpu->attach_output && cpu->cpu->attach_input) {
		cpu->cpu->attach_input(1, read_handler);
		cpu->cpu->attach_output(1, output_handler);
	}
	else {
		widget_set_label(output, "ERRORR!");
		return false;
	}
	clear();
	return true;
}

void c(int index) {
	cpu_handler(index);
}

void gen_radio_list(widget_t *group, list_t *list, int *current, void(*handler)(int index)) {
	group_clear(group, true);
	for (int i = 0; i < list->length; i++) {
		plugin_t *plugin = (plugin_t*)(list->items[i]);
		group_add(group, radio_create(plugin->name, current, i, handler));
	}
}

void update_cpu() {
	list_t *current_cpus = get_plugins("CPU");
	if (current_cpus == NULL) {
		current_cpus = create_list();
	}
	list_insert(current_cpus, 0, &disabled);
	if (cpu_list) {
		list_free(cpu_list);
	}
	cpu_list = current_cpus;
	gen_radio_list(cpus, cpu_list, &current_cpu, c);
}

void frame(float delta) {
	update_cpu();
	if (cpu == &disabled) {
		widget_set_label(output, "No CPU connected");
	}
	else {
		widget_set_label(output, output_buf);
	}
	render_window(window);
}

bool supports(const char *type) {
	return !strcmp(type, "Perpetual") || !strcmp(type, "Display");
}
	
perpetual_plugin_t perpetual = {
	.frame = frame
};

plugin_t plugin = {
	.name = "Serial Monitor",
	.supports = supports,
	.perpetual = &perpetual
};

void input_handler(widget_t *input) {
	char *msg = input_get_text(input);
	{
		char *m = malloc(strlen(msg + 2));
		if (m == NULL) {
			puts("Failed to allocate memory");
			exit(1);
		}
		strcat(strcpy(m, msg), "\n");
		free(msg);
		msg = m;
	}
	for (size_t i = 0; i < strlen(msg); i++) {
		ring_buffer_append(input_buf, msg + i, 1);
		if (cpu->cpu && cpu->cpu->fire_interrupt) {
			cpu->cpu->fire_interrupt(1);
			cpu->cpu->fire_interrupt(2);
		}
	}
	free(msg);
	input_set_text(input, "");
}

PLUGIN_EXPORT void init() {
	window = window_create("Serial Monitor");
	window_min_size(window, 200, 160);
	window_set_pos(window, 0, 20);
	window_initial_size(window, 450, 700);
	menu = menu_create("Control");
	menu_append(menu, menuitem_create("Clear", clear));
	window_append_menu(window, menu);
	output = label_create(NULL);
	input = input_create("", 128, input_handler);
	window_append(window, output);
	window_append(window, input);
	cpu = &disabled;
	cpus = group_create();
	select_cpu = menu_create("Select CPU");
	menu_append(select_cpu, cpus);
	menu_append(menu, submenu_create(select_cpu));
	cpu_list = NULL;
	output_buf = malloc(32 * 1024 * 1024);
	out_size = 0;
	input_buf = ring_buffer_new(1024);
	update_cpu();
	// Attempt to use first none-disabled
	if (cpu_handler(1)) {
		current_cpu = 1;
	}
}

PLUGIN_EXPORT void cleanup() {
	window_destroy(window);
	menu_destroy_all(menu);
	menu_destroy(menu);
	group_clear(cpus, true);
	widget_destroy(cpus);
	menu_destroy(select_cpu);
	widget_destroy(output);
	widget_destroy(input);
	ring_buffer_free(input_buf);
	free(output_buf);
}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin;
}
