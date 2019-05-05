#include "simple_shell.h"
#include <Zany80/Plugin.h>
#include <string.h>

SimpleShell *shell;

perpetual_plugin_t perpetual;
plugin_t plugin;

extern "C" {
	
	PLUGIN_EXPORT void init() {
		shell = new SimpleShell;
		memset(&perpetual, 0, sizeof(perpetual_plugin_t));
		memset(&plugin, 0, sizeof(plugin_t));
		perpetual.frame = [](float delta) {
			shell->frame(delta);
		};
		plugin.name = "Simple Shell";
		plugin.supports = [](const char *type) -> bool {
			return (!strcmp(type, "Shell")) || (!strcmp(type, "Perpetual"));
		};
		plugin.perpetual = &perpetual;
	}
	
	PLUGIN_EXPORT plugin_t *get_interface() {
		return &plugin;
	}
	
	PLUGIN_EXPORT void cleanup() {
		delete shell;
	}
	
}
