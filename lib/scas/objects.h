#ifndef OBJECT_H
#define OBJECT_H
#include "list.h"
#include "expression.h"
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

enum {
    SYMBOL_LABEL,
    SYMBOL_EQUATE
};

typedef struct {
    char *key;
    char *value;
    uint64_t value_length;
} metadata_t;

typedef struct {
    int type;
    char *name;
    uint64_t value;
    uint64_t defined_address;
    int exported;
} symbol_t;

typedef struct {
	char *name;
	size_t line_number;
	char *line;
	int column;
	char *file_name;
} unresolved_symbol_t;

typedef struct {
    tokenized_expression_t *expression;
    uint64_t width;
    uint64_t address;
    uint64_t instruction_address;
    uint64_t base_address;
    int type;
} late_immediate_t;

typedef struct {
    uint64_t line_number;
    uint64_t address;
    uint64_t length;
    char *source_code;
} source_map_entry_t;

typedef struct {
    char *file_name;
    list_t *entries;
} source_map_t;

typedef struct {
    char *name;
    list_t *late_immediates;
    list_t *symbols;
    list_t *source_map;
    list_t *metadata;
    uint8_t *data;
    uint64_t data_length;
    uint64_t data_capacity;
    /* Only used for linking */
    uint64_t final_address;
} area_t;

typedef struct {
    list_t *areas;
    /* Only used for assembly */
    list_t *exports;
    list_t *imports;
    list_t *unresolved;
    bool merged;
} object_t;

object_t *create_object();
void object_free(object_t *object);
area_t *create_area(const char *name);
void merged_area_free(area_t *area);
void area_free(area_t *area);
metadata_t *get_area_metadata(area_t *area, const char *key);
void set_area_metadata(area_t *area, const char *key, char *value, uint64_t value_length);
void append_to_area(area_t *area, uint8_t *data, size_t length);
void insert_in_area(area_t *area, uint8_t *data, size_t length, size_t index);
void delete_from_area(area_t *area, size_t index, size_t length);
void fwriteobj(FILE *file, object_t *object);
object_t *freadobj(FILE *file, const char *name);
void add_source_map(source_map_t *map, int line_number, const char *line, uint64_t address, uint64_t length);
source_map_t *create_source_map(area_t *area, const char *file_name);
void source_map_free(source_map_t *map);

#endif
