#include "simple_shell.h"
#include <Zany80/Plugin.h>
#include <string.h>

SimpleShell *shell;

perpetual_plugin_t perpetual = {
	.frame = [](float delta) {
		shell->frame(delta);
	}
};

plugin_t plugin = {
	.name = "Simple Shell",
	.supports = [](const char *type) -> bool {
		return (!strcmp(type, "Shell")) || (!strcmp(type, "Perpetual"));
	},
	.perpetual = &perpetual
};

extern "C" {
	
	void init() {
		shell = new SimpleShell;
	}
	
	plugin_t *get_interface() {
		return &plugin;
	}
	
	void cleanup() {
		delete shell;
	}
	
}
