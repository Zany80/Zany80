#pragma once

#ifndef NEEDED_PLUGINS
#error Must define NEEDED_PLUGINS !
#endif

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>

#include <cstring>

extern "C" {
	PluginType *getType();
	HardwareType *getHardwareType();
	void postMessage(PluginMessage m);
	bool isSignatureCompatible(const char *sig);
	bool isCategory(const char *cat);
	bool isType(const char *type);
	void init(liblib::Library *plugin_manager);
	void cleanup();
	const char *neededPlugins();
	void emulate(uint64_t cycles);
	void cycle();
	uint64_t getCycles();
	uint8_t getAddressBusSize();
	uint8_t getDataBusSize();
}

PluginType type = Hardware;
HardwareType hw_type = CPU;

PluginType *getType(){
	return &type;
}

HardwareType *getHardwareType() {
	return &hw_type;
}

#ifndef OVERRIDE_SIG
extern const char *signature;
bool isSignatureCompatible(const char *sig) {
	return !strcmp(sig,signature);
}
#endif

#ifndef OVERRIDE_EMULATE
void emulate(uint64_t cycles) {
	for (uint64_t i = 0; i < cycles; i++) {
		cycle();
	}
}
#endif

const char *neededPlugins() {
	return NEEDED_PLUGINS;
}
