#include "instructions.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "log.h"
#include "list.h"
#include "readline.h"
#include "stringop.h"

static __inline uint64_t swapbits(uint64_t p, uint64_t m, int k) {
	uint64_t q = ((p>>k)^p)&m;
	return p^q^(q<<k);
}

uint64_t bitreverse(uint64_t n) {
	static const uint64_t m0 = 0x5555555555555555LLU;
	static const uint64_t m1 = 0x0300c0303030c303LLU;
	static const uint64_t m2 = 0x00c0300c03f0003fLLU;
	static const uint64_t m3 = 0x00000ffc00003fffLLU;
	n = ((n>>1)&m0) | (n&m0)<<1;
	n = swapbits(n, m1, 4);
	n = swapbits(n, m2, 8);
	n = swapbits(n, m3, 20);
	n = (n >> 34) | (n << 30);
	return n;
}

operand_group_t *find_operand_group(instruction_set_t *set, const char *name) {
	int i;
	for (i = 0; i < set->operand_groups->length; ++i) {
		operand_group_t *g = set->operand_groups->items[i];
		if (strcmp(g->name, name) == 0) {
			return g;
		}
	}
	return NULL;
}

operand_t *find_operand(operand_group_t *group, const char *match) {
	int i;
	for (i = 0; i < group->operands->length; ++i) {
		operand_t *o = group->operands->items[i];
		if (strcasecmp(o->match, match) == 0) {
			return o;
		}
	}
	return NULL;
}

instruction_operand_t *find_instruction_operand(instruction_t *inst, char key) {
	int i;
	for (i = 0; i < inst->operands->length; ++i) {
		instruction_operand_t *op = inst->operands->items[i];
		if (op->key == key) {
			return op;
		}
	}
	return NULL;
}

immediate_t *find_instruction_immediate(instruction_t *inst, char key) {
	int i;
	for (i = 0; i < inst->immediate->length; ++i) {
		immediate_t *imm = inst->immediate->items[i];
		if (imm->ref == key) {
			return imm;
		}
	}
	return NULL;
}

operand_group_t *create_operand_group(const char *name) {
	operand_group_t *g = malloc(sizeof(operand_group_t));
	g->name = malloc(strlen(name) + 1);
	strcpy(g->name, name);
	g->operands = create_list();
	return g;
}

operand_t *create_operand(const char *match, uint64_t val, size_t len) {
	operand_t *op = malloc(sizeof(operand_t));
	op->match = malloc(strlen(match) + 1);
	strcpy(op->match, match);
	op->value = val;
	op->width = len;
	return op;
}

bool parse_operand_line(const char *line, instruction_set_t *set) {
	list_t *parts = split_string(line, " \t");
	if (parts->length != 4) {
		fprintf(stderr, "Invalid definition found in instruction set: %s\n", line);
		free_flat_list(parts);
		return false;
	}
	operand_group_t *g = find_operand_group(set, (char *)parts->items[1]);
	if (g == NULL) {
		g = create_operand_group((char *)parts->items[1]);
		list_add(set->operand_groups, g);
	}
	char *end;
	uint64_t val = (uint64_t)strtol((char *)parts->items[3], &end, 2);
	if (*end != '\0') {
		fprintf(stderr, "Invalid definition found in instruction set: %s\n", line);
		free_flat_list(parts);
		return false;
	}
	operand_t *op = create_operand((char *)parts->items[2], val, strlen((char *)parts->items[3]));
	list_add(g->operands, op);
	free_flat_list(parts);
	return true;
}

bool parse_instruction_line(const char *line, instruction_set_t *set) {
	list_t *parts = split_string(line, " \t");
	if (parts->length <= 2) {
		fprintf(stderr, "Invalid definition found in instruction set: %s\n", line);
		free_flat_list(parts);
		return false;
	}
	/* Initialize an empty instruction */
	instruction_t *inst = malloc(sizeof(instruction_t));
	inst->match = malloc(strlen(parts->items[1]) + 1);
	strcpy(inst->match, parts->items[1]);
	inst->operands = create_list();
	inst->immediate = create_list();
	inst->value = 0;
	/* Parse match */
	for (size_t i = 0; i < strlen(inst->match); ++i) {
		if (inst->match[i] == '@') /* Operand */ {
			char key = inst->match[++i];
			i += 2; /* Skip key, < */
			int g_len = strchr(inst->match + i, '>') - (inst->match + i);
			char *g = malloc(g_len + 1);
			int j;
			for (j = 0; inst->match[i] != '>'; ++j) {
				g[j] = inst->match[i++];
			}
			g[j] = '\0';
			instruction_operand_t *op = malloc(sizeof(instruction_operand_t));
			op->key = key;
			op->group = g;
			operand_group_t *group = find_operand_group(set, g);
			op->width = ((operand_t *)group->operands->items[0])->width;
			/* Note: operand shift is not populated yet */
			list_add(inst->operands, op);
		} else if (inst->match[i] == '%' || inst->match[i] == '^' || inst->match[i] == '&') /* Immediate value */ {
			char type = inst->match[i];
			char key = inst->match[++i];
			i += 2; /* Skip key, < */
			size_t width;
			if (type != '&') {
				int g_len = strchr(inst->match + i, '>') - (inst->match + i);
				char *g = malloc(g_len + 1);
				int j;
				for (j = 0; inst->match[i] != '>'; ++j) {
					g[j] = inst->match[i++];
				}
				g[j] = '\0';
				width = atoi(g);
				free(g);
			} else {
				width = 3;
			}
			immediate_t *imm = malloc(sizeof(immediate_t));
			imm->ref = key;
			imm->width = width;
			switch (type) {
			case '%':
				imm->type = IMM_TYPE_ABSOLUTE;
				break;
			case '^':
				imm->type = IMM_TYPE_RELATIVE;
				break;
			case '&':
				imm->type = IMM_TYPE_RESTART;
				break;
			}
			/* Note: immediate value shift is not populated yet */
			list_add(inst->immediate, imm);
		}
	}
	/* Parse value */
	char *_value = malloc(strlen(line) + 1);
	strcpy(_value, line);
	int trimmed_start;
	_value = strip_whitespace(_value, &trimmed_start);
	char *value = _value;
	while (*value++ != ' ') { }
	while (*value++ != ' ') { }
	inst->width = 0;
	int shift = 0;
	for (size_t i = 0; i < strlen(value); ++i) {
		if (value[i] == ' ' || value[i] == '\t') {
			continue;
		}
		if (value[i] == '1') {
			inst->value |= 1 << shift;
			++inst->width;
			++shift;
		} else if (value[i] == '0') {
			++inst->width;
			++shift;
		} else if (value[i] == '@') /* Operand */ {
			char key = value[++i];
			instruction_operand_t *op = find_instruction_operand(inst, key);
			if (op == NULL) {
				fprintf(stderr, "Invalid definition found in instruction set (unknown operand group): %s\n", line);
				free(_value);
				free_flat_list(parts);
				return false;
			}
			op->shift = shift;
			shift += op->width;
			inst->width += op->width;
		} else if (value[i] == '%' || value[i] == '^' || value[i] == '&') /* Immediate value */ {
			char key = value[++i];
			immediate_t *imm = find_instruction_immediate(inst, key);
			if (imm == NULL) {
				fprintf(stderr, "Invalid definition found in instruction set (unknown immediate value): %s\n", line);
				free(_value);
				free_flat_list(parts);
				return false;
			}
			imm->shift = shift;
			shift += imm->width;
			inst->width += imm->width;
		}
	}
	inst->value = bitreverse(inst->value) >> (64 - inst->width);
	list_add(set->instructions, inst);
	free(_value);
	free_flat_list(parts);
	return true;
}

bool handle_line(char *line, instruction_set_t *result) {
	int trimmed_start;
	line = strip_whitespace(line, &trimmed_start);
	if (line[0] == '\0' || line[0] == '#') {
		free(line);
		return true;
	}
	if (strstr(line, "ARCH ") == line) {
		if (result->arch) {
			fprintf(stderr, "Instruction set definition contains two ARCH directives!\n\tAttempted to override '%s' with '%s'\n", result->arch, line + 5);
			free(line);
			return false;
		}
		else {
			result->arch = strdup(line + 5);
			free(line);
			return true;
		}
	}
	if (strstr(line, "OPERAND ") == line) {
		const bool val = parse_operand_line(line, result);
		free(line);
		return val;
	}
	if (strstr(line, "INS ") == line) {
		const bool val = parse_instruction_line(line, result);
		free(line);
		return val;
	}
	fprintf(stderr, "Unrecognized line: %s\n", line);
	free(line);
	return false;
}

instruction_set_t *load_instruction_set(FILE *file) {
	instruction_set_t *result = malloc(sizeof(instruction_set_t));
	result->instructions = create_list();
	result->operand_groups = create_list();
	result->arch = NULL;
	while (!feof(file)) {
		char *line = read_line(file);
		if (!handle_line(line, result)) {
			fprintf(stderr, "Error loading instruction set!\n");
			instruction_set_free(result);
			return NULL;
		}
	}
	return result;
}

instruction_set_t *load_instruction_set_s(const char *set) {
	instruction_set_t *result = malloc(sizeof(instruction_set_t));
	result->instructions = create_list();
	result->operand_groups = create_list();
	result->arch = NULL;
	int offset = 0;
	while (set[offset]) {
		char *line = read_line_s(set, &offset);
		if (!handle_line(line, result)) {
			fprintf(stderr, "Error loading instruction set!\n");
			instruction_set_free(result);
			return NULL;
		}
	}
	return result;
}

void instruction_set_free(instruction_set_t *set) {
	int i, n;
	for (i = 0; i < set->instructions->length; ++i) {
		instruction_t *inst = set->instructions->items[i];
		/* TODO: This leaks a few other things */
		for (int i = 0; i < inst->operands->length; i += 1) {
			instruction_operand_t *op = (instruction_operand_t*)inst->operands->items[i];
			free(op->group);
		}
		free_flat_list(inst->operands);
		free_flat_list(inst->immediate);
		free(inst->match);
		free(inst);
	}
	list_free(set->instructions);

	for (i = 0; i < set->operand_groups->length; ++i) {
		operand_group_t *group = set->operand_groups->items[i];
		for (n = 0; n < group->operands->length; ++n) {
			operand_t *op = group->operands->items[n];
			free(op->match);
		}
		free_flat_list(group->operands);
		free(group->name);
		free(group);
	}
	list_free(set->operand_groups);

	if (set->arch != NULL) {
		free(set->arch);
	}
	free(set);
}
