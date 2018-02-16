#include <Zany80/Generic.hpp>
#include <cstring>
#include <iostream>

liblib::Library *pm;

const char *signature = "";

const char *neededPlugins() {
	return "CPU/z80;RAM/16_8";
}

void init(liblib::Library *plugin_manager) {
	pm = plugin_manager;
}

void postMessage(PluginMessage m) {
	
}
