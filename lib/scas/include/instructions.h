#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdio.h>
#include <stdint.h>
#include "list.h"

enum {
    IMM_TYPE_ABSOLUTE = 0,
    IMM_TYPE_RELATIVE = 1,
    IMM_TYPE_RESTART = 2
};

typedef struct {
    char *arch;
    list_t *instructions;
    list_t *operand_groups;
} instruction_set_t;

typedef struct {
    char *match;
    uint64_t value;
    size_t width;
    list_t *immediate;
    list_t *operands;
} instruction_t;

typedef struct {
    char ref;
    int width;
    int shift;
    int type;
} immediate_t;

typedef struct {
    char *name;
    list_t *operands;
} operand_group_t;

typedef struct {
    char *match;
    size_t width;
    uint64_t value;
} operand_t;

typedef struct {
    char key;
    char *group;
    size_t width;
    size_t shift;
} instruction_operand_t;

instruction_set_t *load_instruction_set(FILE *file);
instruction_set_t *load_instruction_set_s(const char *set);
void instruction_set_free(instruction_set_t *set);
instruction_operand_t *find_instruction_operand(instruction_t *inst, char key);
immediate_t *find_instruction_immediate(instruction_t *inst, char key);
operand_group_t *find_operand_group(instruction_set_t *set, const char *name);
operand_t *find_operand(operand_group_t *group, const char *match);

#endif
