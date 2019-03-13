#include <Zany80/Plugin.h>
#include <stdio.h>

bool supports(const char *feature) {
	return false;
}

plugin_t plugin = {
	.supports = supports,
	.name = "Debug Port"
};

void output_handler(uint8_t value) {
	printf("%c", (char)value);
}

void init() {
	plugin_t *cpu = require_plugin("z80cpp_core");
	if (cpu->cpu && cpu->cpu->attach_output) {
		cpu->cpu->attach_output(2, output_handler);
	}
	else {
		fputs("z80cpp_core CPU structure missing or insufficient", stderr);
	}
}

void cleanup() {}

plugin_t *get_interface() {
	return &plugin;
}
