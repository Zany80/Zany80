#include "bin.h"
#include <stdio.h>
#include <stdint.h>

int output_bin(FILE *f, uint8_t *data, int data_length) {
	fwrite(data, sizeof(uint8_t), data_length, f);
	return 0;
}
