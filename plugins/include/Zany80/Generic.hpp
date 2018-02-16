#pragma once

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>

extern "C" {
	PluginType *getType();
	void postMessage(PluginMessage m);
	bool isSignatureCompatible(const char *sig);
	void init(liblib::Library *plugin_manager);
	void cleanup();
	const char *neededPlugins();
}

extern const char *signature;

PluginType *getType(){
	static PluginType type = Generic;
	return &type;
}

#ifndef OVERRIDE_SIG

#include <string.h>

bool isSignatureCompatible(const char *sig) {
	return strcmp(sig,signature) == 0;
}

#endif
