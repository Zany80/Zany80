#pragma once

#include <fs/ZanyFS.h>

node_t *getNode(node_t *parent, char *name);
node_t *createFile(const zanyfs_t *const file_system);
