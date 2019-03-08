#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <list.h>

typedef uint8_t(*read_handler_t)();
typedef void(*write_handler_t)(uint8_t);

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
	void(*load_rom)(const char *rom);
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
	const char *name;
	// The host program can store ANY TYPE in library
	// Plugins must not use it directly!
	void *library;
	char *path;
	bool (*supports)(const char *functionality);
	perpetual_plugin_t *perpetual;
	cpu_plugin_t *cpu;
	toolchain_plugin_t *toolchain;
	plugin_version_t *version;
} plugin_t;

list_t *get_plugins(const char *type);
list_t *get_all_plugins();
void require_plugin(const char *type);
bool load_plugin(const char *path);
void unload_plugin(plugin_t *plugin);
