#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <scas/list.h>
#include <scas/stringop.h>

#include <liblib/liblib.hpp>
#include <Zany80/API.h>
#include <Zany80/Plugin.h>

#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <IO/IO.h>
using namespace Oryol;

list_t *plugins = nullptr;

void invalid_plugin(const char *path, liblib::Library *invalid) {
	if ((*invalid)["get_interface"]) {
		fprintf(stderr, "Error loading plugin '%s' at path '%s'\n", ((plugin_t*(*)())((*invalid)["get_interface"]))()->name,path);
	}
	else {
		fprintf(stderr, "Error loading plugin at path '%s'\n", path);
	}
	delete invalid;
}

bool validate_plugin_library(liblib::Library *library) {
	#define REQUIRE(x) ((*library)[x] != 0)
	return REQUIRE("get_interface") && REQUIRE("init") && REQUIRE("cleanup");
}

Array<jmp_buf*> envs;
// List of plugins that we are currently attempting to load
Array<String> attempting;

bool load_plugin(const char *path) {
	bool success = false;
	attempting.Add(path);
	liblib::Library *library = new liblib::Library(path);
	if (library->isValid() && validate_plugin_library(library)) {
		jmp_buf env;
		envs.Add(&env);
		if (setjmp(env) == 0) {
			((void(*)())(*library)["init"])();
			plugin_t *plugin = ((plugin_t*(*)())(*library)["get_interface"])();
			plugin->library = library;
			if (plugins == nullptr) {
				plugins = create_list();
			}
			plugin->path = path;
			list_add(plugins, plugin);
			success = true;
		}
		else {
			invalid_plugin(path, library);
		}
		envs.Erase(envs.FindIndexLinear(&env));
	}
	else {
		invalid_plugin(path, library);
	}
	attempting.Erase(attempting.FindIndexLinear(path));
	return success;
}

void unload_plugin(plugin_t *plugin) {
	bool found = false;
	for (int i = 0; i < plugins->length; i++) {
		if (plugins->items[i] == plugin) {
			list_del(plugins, i);
			found = true;
			break;
		}
	}
	o_assert_dbg(found);
	liblib::Library *library = (liblib::Library*)plugin->library;
	plugin->library = nullptr;
	((void(*)())((*library)["cleanup"]))();
	delete library;
	// PLUGIN IS NO LONGER VALID!
	if (plugins->length == 0) {
		list_free(plugins);
		plugins = nullptr;
	}
}

list_t *get_all_plugins() {
	list_t *p = create_list();
	if (plugins != nullptr)
		list_cat(p, plugins);
	return p;
}

list_t *get_plugins(const char *type) {
	if (plugins == nullptr)
		return nullptr;
	list_t *found = create_list();
	for (int i = 0; i < plugins->length; i++) {
		plugin_t *plugin = (plugin_t*)(plugins->items[i]);
		if (plugin->supports(type)) {
			list_add(found, plugin);
		}
	}
	return found;
}

bool require_plugin(const char *type) {
	if (plugins != nullptr) {
		for (int i = 0; i < plugins->length; i++) {
			plugin_t *plugin = (plugin_t*)plugins->items[i];
			if (plugin->supports(type)) {
				return true;
			}
		}
	}
	// Try to load
	String uri = IO::ResolveAssigns("plugins:");
	list_t *libraries = read_directory(uri.AsCStr() + 8);
	bool loaded = false;
	for (int i = 0; i < libraries->length; i++) {
		const char *path = (char*)libraries->items[i];
		StringBuilder s(path);
		int index = s.FindLastOf(0, EndOfString, ".");
		if (index == InvalidIndex)
			continue;
		String name = s.GetSubString(0, index);
		s.Set("plugins:");
		s.Append(name);
		if (attempting.FindIndexLinear(s.GetString()) != InvalidIndex)
			continue;
		if (plugins != nullptr) {
			bool found = false;
			for (int j = 0; j < plugins->length; j++) {
				if (!strcmp(((plugin_t*)plugins->items[j])->path, s.AsCStr())) {
					found = true;
					break;
				}
			}
			if (found)
				continue;
		}
		fprintf(stderr, "Trying %s (%s) as a %s\n", path, s.AsCStr(), type);
		attempting.Add(path);
		liblib::Library *library = new liblib::Library(s.GetString());
		if (library->isValid() && validate_plugin_library(library)) {
			jmp_buf env;
			envs.Add(&env);
			if (setjmp(env) == 0) {
				((void(*)())(*library)["init"])();
				plugin_t *plugin = ((plugin_t*(*)())(*library)["get_interface"])();
				if (!plugin->supports(type)) {
					fprintf(stderr, "Plugin '%s' does not match type '%s', unloading...\n", plugin->name, type);
					((void(*)())(*library)["cleanup"])();
					delete library;
				}
				else {
					plugin->library = library;
					if (plugins == nullptr) {
						plugins = create_list();
					}
					plugin->path = path;
					list_add(plugins, plugin);
					loaded = true;
				}
			}
			else {
				invalid_plugin(path, library);
			}
			envs.Erase(envs.FindIndexLinear(&env));
		}
		else {
			invalid_plugin(path, library);
		}
		attempting.Erase(attempting.FindIndexLinear(path));
		if (loaded)
			break;
	}
	free_flat_list(libraries);
	if (!loaded) {
		if (envs.Size() != 0)
			longjmp(*envs.Back(), 1);
	}
	return true;
}
