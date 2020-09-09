#include "SIMPLE/Plugin.h"
#include "SIMPLE/API.h"

plugin_version_t version_info = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.str = "Alpha 1.0.0"
};

bool supports(const char *type) {
	return false;
}

plugin_t interface = {
	.name = "DragonFruit Editor",
	.supports = supports,
	.version = &version_info
};

/**
 * This plugin is *not* a perpetual - it doesn't render an editor widget every
 * frame. Instead, it modifies the root window at startup, and then removes
 * itself from that window when shutting down.
 */

widget_t *editor;

PLUGIN_EXPORT void init() {
	
}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &interface;
}

PLUGIN_EXPORT void cleanup() {
	
}