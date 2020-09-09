#include <stdint.h>
#include <stdbool.h>

#ifndef SERIAL_BUF_SIZE
#define SERIAL_BUF_SIZE 32 * 1024
#endif

/// Reads a character from the input buffer. The upper 24 bits are zeroed.
uint32_t serial_read();
/// Writes to the serial monitor's output buffer. The upper 24 bits are ignored.
void serial_write(uint32_t value);
/// Clears the serial monitor's output buffer.
void serial_clear_output();
/// Clears the serial monitor's input buffer - this does not affect data which
/// has already been read.
void serial_clear_input();
/// Initializes the serial monitor, and registers it with SIMPLE.
void serial_init();
/// Frees up all resources used by the serial monitor.
void serial_deinit();
/// Moves the serial monitor's menu into the root window under the title Serial,
/// or removes it from the root window. If the monitor is docked, this has no
/// effect.
void serial_toggle_root_menu();
/// Moves the serial monitor into the root window - or, if it is already there,
/// pops it out. The menu is moved to the new target.
void serial_toggle_root();
extern bool serial_is_docked;
