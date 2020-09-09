#include <stdint.h>

#ifndef SERIAL_BUF_SIZE
#define SERIAL_BUF_SIZE 32 * 1024
#endif

uint32_t serial_read();
void serial_write(uint32_t value);
void serial_clear();
void serial_init();
void serial_deinit();
