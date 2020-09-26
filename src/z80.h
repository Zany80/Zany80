#ifndef ZANY80_Z80_H_
#define ZANY80_Z80_H_

#include <stddef.h>
#include <stdint.h>

void z80_init(void);
void z80_deinit(void);
void z80_reset(void);
void z80_unhalt(void);
void z80_halt(void);
void z80_load(uint8_t *buf, size_t len);
void z80_jump(uint16_t target);

#endif // ZANY80_Z80_H_

