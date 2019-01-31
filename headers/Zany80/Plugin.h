#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <list.h>

//~ #include <functional>
//~ typedef std::function<uint8_t()> read_handler_t;
//~ typedef std::function<void(uint8_t)> write_handler_t;

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
} cpu_plugin_t;

typedef struct {
	const char *name;
	bool (*supports)(const char *functionality);
	perpetual_plugin_t *perpetual;
	cpu_plugin_t *cpu;
	// The host program can store ANY TYPE here
	// Plugins must not use it directly!
	void *library;
	const char *path;
} plugin_t;

list_t *get_plugins(const char *type);
list_t *get_all_plugins();
bool require_plugin(const char *type);
bool load_plugin(const char *path);
void unload_plugin(plugin_t *plugin);
