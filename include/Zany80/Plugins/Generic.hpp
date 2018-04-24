#pragma once

#ifndef NEEDED_PLUGINS
#error Must define NEEDED_PLUGINS !
#endif

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>

extern "C" {
	PluginType *getType();
	void postMessage(PluginMessage m);
	const char *neededPlugins();
	bool isCategory(const char *sig);
	bool isType(const char *sig);
}

PluginType *getType(){
	static PluginType type = Generic;
	return &type;
}

const char *neededPlugins() {
	return NEEDED_PLUGINS;
}
