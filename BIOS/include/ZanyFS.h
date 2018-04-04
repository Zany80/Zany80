#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <fs/malloc.h>

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

typedef struct {
	char zany[4];
	node_t root;
	header_t *heap_begin;
	header_t *heap_end;
	header_t *free;
} zanyfs_t;

void *malloc(size_t size, zanyfs_t *file_system);
