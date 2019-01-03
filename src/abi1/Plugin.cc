#define ZANY80_ABI 1

#include <Zany80/Zany80.h>
#include <Zany80/API.h>
#include <Zany80/Plugin.h>
#include <liblib/liblib.hpp>
#include <IO/IO.h>

#include <config.h>

Map<String, Plugin*> plugins;

#ifndef ORYOL_EMSCRIPTEN

static bool cancel_construction;

extern Array<String> available_plugins;

Plugin *construct(String name) {
	Plugin *plugin = nullptr;
	cancel_construction = false;
	liblib::Library plugin_library(name);
	if (plugin_library.isValid()) {
		char*(*getName)() = (char*(*)())plugin_library["getName"];
		if (getName) {
			Log::Dbg("Constructing %s...\n", getName());
		}
		else {
			StringBuilder error;
			error.AppendFormat(512, "%s does not implement `char *getName()`", name.AsCStr());
			reportError(error.AsCStr());
		}
		Plugin *(*make)() = (Plugin*(*)())plugin_library["make"];
		if (make != nullptr) {
			plugin = make();
		}
		if (cancel_construction) {
			delete plugin;
			plugin = nullptr;
			StringBuilder s("Construction of ");
			s.Append(getName());
			s.Append(" aborted.");
			reportError(s.AsCStr());
		}
	}
	return plugin;
}

bool requirePlugin(String type) {
	if (getPlugins(type).Size() == 0) {;
		Map<String, String> plugin_types = {
			{ "CPU", "z80cpp_core" },
			{ "Toolchain", "assembler" }
		};
		if (plugin_types.Contains(type)) {
			StringBuilder pname("plugins:");
			pname.Append(plugin_types[type]);
			spawnPlugin(pname.GetString());
			if (!cancel_construction) {
				return true;
			}
		}
		cancel_construction = true;
		return false;
	}
	return true;
}

void doSpawn(String name) {
	Plugin *p = construct(name);
	if (p != nullptr)
		plugins.Add(name, p);
}

void spawnPlugin(String name) {
	doSpawn(name);
}

#else

bool requirePlugin(String type) {
	// Since all plugins are instantiated manually, don't try to enforce the
	// normal dependencies.
	return true;
}

#endif

Array<Plugin*> getPlugins(String type) {
	Array<Plugin*> _plugins;
	for (KeyValuePair<String, Plugin*> p : plugins) {
		if (p.value->supports(type)) {
			_plugins.Add(p.value);
		}
	}
	return _plugins;
}

String getPluginName(Plugin *plugin) {
	for (KeyValuePair<String, Plugin*> p : plugins) {
		if (p.value == plugin) {
			return p.key;
		}
	}
	return "Not Found";
}
