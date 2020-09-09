#ifndef ERRORS_H
#define ERRORS_H
#include "list.h"
#include <stdint.h>
#include <stddef.h>

enum {
    ERROR_INVALID_INSTRUCTION = 1,
    ERROR_VALUE_TRUNCATED,
    ERROR_INVALID_SYNTAX,
    ERROR_INVALID_DIRECTIVE,
    ERROR_UNKNOWN_SYMBOL,
    ERROR_BAD_FILE,
    ERROR_TRAILING_END,
    ERROR_DUPLICATE_SYMBOL,
};

enum {
	WARNING_NO_EFFECT = 1,
};

typedef struct {
    int code;
    size_t line_number;
    char *file_name;
    char *line;
    char *message;
    size_t column;
} error_t;

typedef struct {
    int code;
    size_t line_number;
    char *file_name;
    char *line;
    char *message;
    size_t column;
} warning_t;

const char *get_error_string(error_t *error);
const char *get_warning_string(warning_t *warning);
void add_error(list_t *errors, int code, size_t line_number, const char *line,
        int column, const char *file_name, ...);
void add_error_from_map(list_t *errors, int code, list_t *maps, uint64_t
        address, ...);
void add_warning(list_t *warnings, int code, size_t line_number,
		const char *line, int column, const char *file_name, ...);

#endif
