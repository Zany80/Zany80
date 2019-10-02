#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	char *buf;
	size_t length;
	bool valid;
} SIMPLEFile;

void data_register(const char *const name, const char *const full_path, const char *const version);
SIMPLEFile simple_data_file(const char *const name, const char *const version);
void simple_data_free(SIMPLEFile f);