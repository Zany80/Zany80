#define NEEDED_PLUGINS ""
#include <Zany80/Generic.hpp>
#include <cstring>
#include <iostream>

liblib::Library *pm;

const char *signature = "";

const char *neededPlugins() {
	return "CPU/z80;RAM/16_8";
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "init")) {
		plugin_manager = (liblib::Library*)m.context;
	}
}
