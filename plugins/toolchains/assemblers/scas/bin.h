#ifndef _BIN_H
#define _BIN_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
#endif
int output_bin(FILE *f, uint8_t *data, int data_length);

#endif
