#pragma once

#include <stdbool.h>

typedef struct {
	const char *data;
	int size;
	bool executable;
} file_t;

typedef struct node_t node_t;

typedef struct {
	node_t **children;
} directory_t;

struct node_t {
	const char *name;
	char is_file;
	union {
		file_t *file;
		directory_t dir;
	};
	node_t *parent;
};
