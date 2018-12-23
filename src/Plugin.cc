#include <Zany80/Zany80.h>
#include <Zany80/Plugin.h>
#include <liblib/liblib.hpp>
#include <IO/IO.h>

#include <config.h>

Map<String, PPTR> plugins;

#ifndef ORYOL_EMSCRIPTEN

PPTR construct(String name) {
	liblib::Library plugin(name);
	if (plugin.isValid()) {
		char*(*getName)() = (char*(*)())plugin["getName"];
		if (getName) {
			Log::Dbg("Constructing %s...\n", getName());
		}
		else {
			Log::Error("Error getting name of under construction plugin.\n");
		}
		Plugin *(*make)() = (Plugin*(*)())plugin["make"];
		if (make != nullptr) {
			return make();
		}
	}
	return nullptr;
}

void doSpawn(String name) {
	PPTR p = construct(name);
	if (p != nullptr)
		plugins.Add(name, p);
}

void spawnPlugin(String name) {
	doSpawn(name);
}

#endif

Array<PPTR> getPlugins(String type) {
	Array<PPTR> _plugins;
	for (KeyValuePair<String, PPTR> p : plugins) {
		if (GET_PPTR(p.value)->supports(type)) {
			_plugins.Add(p.value);
		}
	}
	return _plugins;
}

String getPluginName(PPTR plugin) {
	for (KeyValuePair<String, PPTR> p : plugins) {
		if (GET_PPTR(p.value) == GET_PPTR(plugin)) {
			return p.key;
		}
	}
	return "Not Found";
}
