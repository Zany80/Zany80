#pragma once

#include <stddef.h>

#include <fs/ZanyFS.h>

typedef struct header header_t;

struct header {
	header_t *next;
	header_t *next_free;
};
