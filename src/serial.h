#ifndef ZANY_SERIAL_H
#define ZANY_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

#ifndef SERIAL_BUF_SIZE
#define SERIAL_BUF_SIZE 320 * 1024
#endif

/// Reads a character from the input buffer. The upper 24 bits are zeroed.
uint32_t serial_read();
/// Writes to the serial port's output buffer. The upper 24 bits are ignored.
void serial_write(uint32_t value);
/// Clears the serial port's output buffer.
void serial_clear_output();
/// Clears the serial port's input buffer - this does not affect data which
/// has already been read.
void serial_clear_input();
/// Initializes the serial port, and registers it with SIMPLE. If docked is true
/// then the port will begin in the root window.
void serial_init(bool docked);
/// Frees up all resources used by the serial port.
void serial_deinit();
/// Moves the serial port into the root window - or, if it is already there,
/// pops it out. The menu is moved to the new target.
void serial_toggle_root();
extern bool serial_is_docked;

/// Writes the full message to the serial port's output buffer. If len is -1,
/// the length is obtained via strlen; otherwise, it is used as given.
void serial_write_all(const char *const msg, int32_t len);

#endif
