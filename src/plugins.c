#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <inttypes.h>

#include "SIMPLE/3rd-party/cfgpath.h"
#include "SIMPLE/3rd-party/stretchy_buffer.h"
#include "SIMPLE/scas/list.h"
#include "SIMPLE/scas/stringop.h"
#include "SIMPLE/API.h"
#include "SIMPLE/Plugin.h"
#include "SIMPLE/repository.h"

static list_t *plugins = NULL;

plugin_config_t config_load() {
	plugin_config_t c = malloc(sizeof(struct plugin_config));
	if (c == NULL) {
		puts("OOM");
		exit(1);
	}
	c->known_plugins = 0;
	c->plugins = NULL;
	char path[MAX_PATH];
	get_user_config_folder(path, MAX_PATH, "SIMPLE");
	strcat(path, "plugins.conf");
	if (access(path, F_OK) == 0) {
		xml_document_t doc = h_document_read(path);
		node_for_each(document_get_root(doc), "Plugin", plugin, {
			plugin_descriptor_t descriptor;
			descriptor.name = strdup(node_get_attribute(plugin, "name"));
			descriptor.path = strdup(node_get_attribute(plugin, "path"));
			descriptor.version = strdup(node_get_attribute(plugin, "version"));
			descriptor.repo = strdup(node_get_attribute(plugin, "repo"));
			descriptor.settings = NULL;
			node_for_each(plugin, NULL, setting, {
				sb_push(descriptor.settings, strdup(node_get_name(setting)));
				sb_push(descriptor.settings, strdup(node_get_value(setting)));
			});
			sb_push(c->plugins, descriptor);
			c->known_plugins++;
		});
		document_destroy(doc);
	}
	return c;
}

void config_free(plugin_config_t c) {
	for (size_t i = 0; i < c->known_plugins; i++) {
		free(c->plugins[i].name);
		free(c->plugins[i].path);
		free(c->plugins[i].repo);
		free(c->plugins[i].version);
		for (int i = 0; i < sb_count(c->plugins[i].settings); i++) {
			if (c->plugins[i].settings[i] != NULL) {
				free(c->plugins[i].settings[i]);
			}
		}
		sb_free(c->plugins[i].settings);
	}
	sb_free(c->plugins);
	free(c);
}

void config_save(plugin_config_t config) {
	xml_document_t doc = h_document_create("PluginConfiguration");
	xml_node_t root = document_get_root(doc);
	for (size_t i = 0; i < config->known_plugins; i++) {
		xml_node_t p = node_create(doc, "Plugin", NULL);
		node_set_attribute(p, "path", config->plugins[i].path);
		node_set_attribute(p, "name", config->plugins[i].name);
		node_set_attribute(p, "repo", config->plugins[i].repo);
		node_set_attribute(p, "version", config->plugins[i].version);
		for (int j = 0; j < sb_count(config->plugins[i].settings); j += 2) {
			node_append_child(p, node_create(doc, config->plugins[i].settings[j], config->plugins[i].settings[j + 1]));
		}
		node_append_child(root, p);
	}
	char path[MAX_PATH];
	get_user_config_folder(path, MAX_PATH, "SIMPLE");
	strcat(path, "plugins.conf");
	if (document_write(doc, path)) {
		simple_log(SL_DEBUG, "Plugin configuration saved to %s successfully!\n", path);
	}
	else {
		simple_report_error("Error saving plugin configuration!");
	}
	document_destroy(doc);
}

widget_t *get_plugin_manager();

static int discard = -1;

static void load_plugin(int _index) {
	size_t index = _index;
	plugin_config_t config = config_load();
	bool failed = true;
	if (index >= config->known_plugins) {
		simple_report_error("Invalid index received! (This probably means the plugin configuration was manually altered)");
	}
	else {
		void *lib = dlopen(config->plugins[index].path, RTLD_LAZY | RTLD_LOCAL);
		if (lib) {
			void (*init)() = dlsym(lib, "init");
			plugin_t*(*get_interface)() = dlsym(lib, "get_interface");
			void (*cleanup)() = dlsym(lib, "cleanup");
			if (init != NULL && get_interface != NULL && cleanup != NULL) {
				init();
				plugin_t *interface = get_interface();
				if (interface != NULL && interface->supports != NULL && interface->name != NULL) {
					interface->library = lib;
					if (plugins == NULL) {
						plugins = create_list();
					}
					interface->path = strdup(config->plugins[index].path);
					list_add(plugins, interface);
					failed = false;
					simple_log(SL_INFO, "%s-%s at %s loaded successfully\n", interface->name, config->plugins[index].version, interface->path);
				}
			}
			if (failed) {
				dlclose(lib);
			}
			else {
				generate_plugin_manager();
			}
		}
	}
	if (failed) {
		simple_report_error("Failed to load plugin!");
	}
	config_free(config);
	discard = -1;
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
	if (found) {
		free(plugin->path);
		void *lib = plugin->library;
		((void(*)())dlsym(lib, "cleanup"))();
		dlclose(lib);
	}
	else {
		simple_report_error("Unable to handle plugin unload request: specified plugin not found!");
	}
	discard = -1;
}

static void _unload_plugin(int _index) {
	size_t index = _index;
	plugin_config_t config = config_load();
	bool failed = true;
	if (index >= config->known_plugins) {
		simple_report_error("Invalid index received! (This probably means the plugin configuration was manually altered)");
	}
	else {
		// find the plugin with the right path
		const char *const path = config->plugins[index].path;
		// don't need to check if plugins isn't NULL; this function is only
		// callable from a UI button which is only visible when plugins isn't
		// null
		for (int i = 0; i < plugins->length; i++) {
			plugin_t *p = plugins->items[i];
			if (!strcmp(p->path, path)) {
				unload_plugin(p);
				failed = false;
				break;
			}
		}
	}
	if (failed) {
		simple_report_error("Unable to find plugin for removal!");
	}
	else {
		simple_log(SL_DEBUG, "Plugin removed successfully.\n");
		generate_plugin_manager();
	}
	config_free(config);
}

void unload_all_plugins() {
	while (plugins != NULL && plugins->length > 0) {
		unload_plugin(plugins->items[0]);
	}
}

list_t *h_get_all_plugins() {
	list_t *p = create_list();
	if (plugins != NULL)
		list_cat(p, plugins);
	return p;
}

list_t *h_get_plugins(const char *type) {
	if (plugins == NULL)
		return NULL;
	list_t *found = create_list();
	for (int i = 0; i < plugins->length; i++) {
		plugin_t *plugin = plugins->items[i];
		if (plugin->supports(type)) {
			list_add(found, plugin);
		}
	}
	return found;
}


void prep_clone(char *const);
void prep_update(char *const);
static widget_t *repo_input;

void repo_add() {
	char *repository_url = input_get_text(repo_input);
	simple_log(SL_INFO, "Adding plugin repository: %s\n", repository_url);
	prep_clone(repository_url);
}

void repo_add_indirect(widget_t *input) {
	repo_add();
}

static void l_repository_update(int _index) {
	plugin_config_t config = config_load();
	size_t index = _index;
	if (index >= config->known_plugins) {
		simple_report_error("Invalid index received: has the configuration been changed manually while SIMPLE was running?");
	}
	else {
	    	prep_update(strdup(config->plugins[index].repo));
	}
	discard = -1;
	config_free(config);
}

void generate_plugin_manager() {
	int index = 0;
	widget_t *pm_group = get_plugin_manager();
	group_clear(pm_group, true);
	group_add(pm_group, label_set_wrapped(label_create("Enter a repository to source for plugins"), true));
	group_add(pm_group, repo_input = input_create("", 512, repo_add_indirect));
	input_set_text(repo_input, "https://github.com/simple-platform/limn1k-ide");
	group_add(pm_group, menuitem_create("Add", repo_add));
	plugin_config_t config = config_load();
	const char **shown_repos = NULL;
	for (size_t i = 0; i < config->known_plugins; i++) {
		bool already_loaded = false;
		plugin_descriptor_t d = config->plugins[i];
		if (plugins != NULL) {
			for (int j = 0; j < plugins->length; j++) {
				plugin_t *p = plugins->items[j];
				if (strcmp(d.path, p->path) == 0) {
					already_loaded = true;
					break;
				}
			}
		}
		bool already_shown = false;
		for (int i = 0; i < sb_count(shown_repos); i++) {
			if (!strcmp(d.repo, shown_repos[i])) {
				already_shown = true;
				break;
			}
		}
		if (!already_shown) {
			sb_push(shown_repos, d.repo);
			char *buf = malloc(7 + strlen(d.repo) + 1);
			sprintf(buf, "Update %s", d.repo);
			group_add(pm_group, radio_create(buf, &discard, i, l_repository_update));
		}
		char *s = d.name ? d.name : d.path;
		char *buf = malloc((already_loaded ? 7 : 5) + strlen(s) + 16 + 1);
		sprintf(buf, "%s %s##%" PRIu64, already_loaded ? "Unload" : "Load", s, i);
		strcat(strcpy(buf, already_loaded ? "Unload " : "Load "), s);
		group_add(pm_group, radio_create(buf, &discard, index++, 
				already_loaded ? _unload_plugin : load_plugin));
		free(buf);
	}
	sb_free(shown_repos);
	config_free(config);
}

void register_plugin(const char *const plugin_path, const char *const repo_path, const char *const version) {
	plugin_config_t configuration = config_load();
	bool known = false;
	bool valid = false;
	char *name = NULL;
	const char *cur_ver = NULL;
	size_t index;
	for (size_t i = 0; i < configuration->known_plugins; i++) {
		if (!strcmp(configuration->plugins[i].path, plugin_path)) {
			known = true;
			cur_ver = configuration->plugins[i].version;
			index = i;
			break;
		}
	}
	void *lib = dlopen(plugin_path, RTLD_LAZY | RTLD_LOCAL);
	if (lib != NULL) {
		void(*init)() = dlsym(lib, "init");
		if (init) {
			init();
			plugin_t*(*get_interface)() = dlsym(lib, "get_interface");
			if (get_interface) {
				plugin_t *p = get_interface();
				if (p) {
					valid = true;
					name = strdup(p->name);
				}
			}
			void(*cleanup)() = dlsym(lib, "cleanup");
			if (cleanup) {
				cleanup();
			}
		}
		dlclose(lib);
	}
	if (valid) {
		if (known && !strcmp(cur_ver, version)) {
			simple_report_error("The target version is already installed!");
			simple_log(SL_ERROR, "Version '%s' of '%s' is already installed!\n", version, configuration->plugins[index].name);
		}
		else {
			if (known) {
				free(configuration->plugins[index].path);
				if (configuration->plugins[index].version != NULL) {
					free(configuration->plugins[index].version);
				}
				configuration->plugins[index].path = strdup(plugin_path);
				configuration->plugins[index].version = version ? strdup(version) : NULL;
			}
			else {
				plugin_descriptor_t descriptor;
				descriptor.path = strdup(plugin_path);
				descriptor.repo = repo_path ? strdup(repo_path) : NULL;
				descriptor.version = version ? strdup(version) : NULL;
				descriptor.name = name ? strdup(name) : name;
				descriptor.settings = NULL;
				sb_push(configuration->plugins, descriptor);
				configuration->known_plugins++;
				simple_log(SL_DEBUG, "Plugin '%s' version '%s' registered successfully!\n", plugin_path, version);
			}
			config_save(configuration);
			generate_plugin_manager();
		}
	}
	if (name) {
		free(name);
	}
	config_free(configuration);
}
