#pragma once

#ifdef _WIN32
#pragma comment(lib, "mincore_downlevel.lib")   // Support OS older than SDK
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "internal/dllports.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <SIMPLE/scas/list.h>

typedef uint32_t(*read_handler_t)();
typedef void(*write_handler_t)(uint32_t);

typedef struct {
	void (*frame)(float delta);
} perpetual_plugin_t;

typedef struct {
	const char *name;
	uint32_t value;
} register_value_t;

typedef struct {
	list_t*(*get_registers)();
	uint64_t(*get_cycle_count)();
	void(*execute)(uint32_t cycles);
	void(*attach_output)(uint32_t port, write_handler_t handler);
	void(*attach_input)(uint32_t port, read_handler_t handler);
	void(*reset)();
	void(*fire_interrupt)(uint8_t irq);
	bool(*load_rom)(const char *rom);
} cpu_plugin_t;

typedef struct {
	const char *source_ext;
	const char *target_ext;
} toolchain_conversion_t;

typedef struct {
	list_t *(*get_conversions)();
	// Buffer should be a pointer to an UNINITIALIZED char*. The convert function
	// mallocs and reallocs as needed.
	int (*convert)(list_t *sources, const char *target, char **buffer);
} toolchain_plugin_t;

typedef struct {
	unsigned int major;
	unsigned int minor;
	unsigned int patch;
	const char *str;
} plugin_version_t;

typedef struct {
	// The host program can store ANY TYPE in library
	// Plugins must not use it directly!
	void *library;
	// *should* contain path to library file, but that's not guaranteed. If the
	// value is *changed*, then it's safe to use it (e.g. if you set path to 
	// null by default, and it's not null, it's safe to use it as a path to the
	// plugin)
	char *path;
	const char *name;
	bool (*supports)(const char *functionality);
	plugin_version_t *version;
	perpetual_plugin_t *perpetual;
	cpu_plugin_t *cpu;
	toolchain_plugin_t *toolchain;
} plugin_t;

SIMPLE_DLL list_t *h_get_plugins(const char *type);
SIMPLE_DLL list_t *h_get_all_plugins();
SIMPLE_DLL void unload_plugin(plugin_t *plugin);
// This function can't be reasonably called from any plugin - doing so would
// result in itself being unloaded, and returning from this function would crash
void unload_all_plugins();

// Only the path is strictly mandatory! This allows for easy manual editing of
// the config file, and makes it easier to use e.g. local plugins or avoid
// updating
typedef struct {
	char *name;
	char *path;
	char *version;
	// needed so that we know where to check for updates and where to direct bug
	// reports, etc
	char *repo;
	// plugin-specific settings managed by SIMPLE. Stored as key1,value1,key2,
	// value2,...keyn,valuen
	char **settings;
} plugin_descriptor_t;

typedef struct plugin_config {
	size_t known_plugins;
	plugin_descriptor_t *plugins;
} *plugin_config_t;

// Explicitly *not* set with SIMPLE_DLL - on Windows, that means these cannot be
// called from plugins; should probably find a way to accomplish the same under
// Linux
// Or, possibly, just allow plugins to add other plugins. There's not really
// much harm that can be added from these functions alone; these are still
// native plugins and as such capable of widespread damage, giving them the
// ability to register other plugins is actually quite harmless.
plugin_config_t config_load();
void config_free(plugin_config_t c);
void config_save(plugin_config_t c);
void register_plugin(const char *const plugin_path, const char *const repo_path, const char *const version);
void generate_plugin_manager();

#ifdef __cplusplus
}
#endif
