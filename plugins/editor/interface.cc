#include <Zany80/Plugin.h>
#include "Editor.h"

Editor *editor;

perpetual_plugin_t perpetual;
plugin_t plugin;

extern "C" {
	
	PLUGIN_EXPORT void init() {
		memset(&perpetual, 0, sizeof(perpetual_plugin_t));
		perpetual.frame = [](float delta) {
			editor->frame(delta);
		};
		memset(&plugin, 0, sizeof(plugin_t));
		plugin.name = "Editor";
		plugin.supports = [](const char *type) -> bool {
			return (!strcmp(type, "Perpetual")) || (!strcmp(type, "Editor"));
		};
		plugin.perpetual = &perpetual;
		editor = new Editor;
	}
	
	PLUGIN_EXPORT plugin_t *get_interface() {
		return &plugin;
	}
	
	PLUGIN_EXPORT void cleanup() {
		delete editor;
	}
	
}
