#include <ZanyFS.h>

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

node_t *getNode(const node_t *parent, const char *name) {
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
