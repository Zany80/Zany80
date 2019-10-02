#pragma once

#include <stdbool.h>

void repository_update(const char *const path);
void repository_register(const char *const path, bool update);