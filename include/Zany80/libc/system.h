#pragma once

#define frames * 1
#define seconds * 60 frames
#define minutes * 60 seconds
#define hours * 60 minutes

#define TITLE(x) void rom_name() __naked { \
	__asm                                  \
	.asciiz x                              \
	__endasm;                              \
}

#include <stdbool.h>

void halt();
