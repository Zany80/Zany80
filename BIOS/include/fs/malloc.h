#pragma once

typedef struct header header_t;

struct header {
	header_t *next;
	header_t *next_free;
};
