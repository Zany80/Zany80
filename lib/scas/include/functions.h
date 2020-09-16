#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "list.h"
#include "objects.h"
#include <stdint.h>

typedef struct {
    area_t *area;
    char *name;
    char *start_symbol;
    char *end_symbol;
    /* Used during linking: */
    int precious;
    uint64_t start_address;
    uint64_t end_address;
} function_metadata_t;

list_t *decode_function_metadata(area_t *area, char *value);
char *encode_function_metadata(list_t *metadata, uint64_t *value_length);
void remove_unused_functions(object_t *object);

#endif
