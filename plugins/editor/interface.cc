#include <Zany80/Plugin.h>
#include "Editor.h"

Editor *editor;

perpetual_plugin_t perpetual = {
	.frame = [](float delta) {
		editor->frame(delta);
	}
};

plugin_t plugin = {
	.name = "Editor",
	.supports = [](const char *type) -> bool {
		return (!strcmp(type, "Perpetual")) || (!strcmp(type, "Editor"));
	},
	.perpetual = &perpetual
};

extern "C" {
	
	void init() {
		editor = new Editor;
	}
	
	plugin_t *get_interface() {
		return &plugin;
	}
	
	void cleanup() {
		delete editor;
	}
	
}
