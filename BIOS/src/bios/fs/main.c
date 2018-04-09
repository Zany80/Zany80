#include <fs/ZanyFS.h>

#include <string.h>

int node_count(const node_t *directory) {
	int nodes;
	node_t *current;
	if (directory->is_file) {
		return 0;
	}
	for (nodes = 0, current = directory->dir.children[0]; current != 0; nodes++) {
		current = directory->dir.children[nodes];
	}
	return nodes;
}

char *getName(const node_t *address, char *target) {
	strcpy(target, address->name);
	return target;
}

node_t *getNodeFromIndex(const node_t *parent, const int index) {
	if (parent->is_file || index > node_count(parent)) {
		return 0;
	}
	return parent->dir.children[index];
}

node_t *getNode(const node_t *const parent, const char *const name) {
	int i;
	if (!parent->is_file) {
		int length = node_count(parent);
		for (i = 0; i < length; i++) {
			if (strequal(parent->dir.children[i]->name, name)) {
				return parent->dir.children[i];
			}
		}
	}
	return 0;
}

typedef struct {
	node_t node;
	file_t file;
} file_node_t;

node_t *createFile(const zanyfs_t *const file_system, const char *const name, node_t *const parent) {
	file_node_t *file_node = (file_node_t*)malloc(sizeof(file_node_t), file_system);
	if (file_node != 0) {
		file_node->node.name = name;
		file_node->node.is_file = true;
		file_node->node.file = &file_node->file;
		file_node->node.parent = parent;
	}
	return &file_node->node;
}
