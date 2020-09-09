#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "stringop.h"
#include "match.h"
#include "log.h"
#include "instructions.h"

/* 
 * Used to get the value provided in code for an operand or immediate value 
 * For example, given this match:
 *     	LD_A-,-(-%A<16>-)
 * And given this code:
 * 		ld a, (0x100 + 2)
 * get_operand_string could be used to extract "a" and "0x100 + 2"
 */
char *get_operand_string(instruction_t *inst, int *i, const char *code, int j) {
	char delimiter = '\0';
	char *res;
	while (inst->match[*i]) {
		if (inst->match[*i] == '-' || inst->match[*i] == '_') {
			++*i;
		} else {
			delimiter = inst->match[*i];
			break;
		}
	}
	if (delimiter == '\0') {
		return strdup(code + j);
	}
	const char *significant_delimiters = "%@&^";
	if (strchr(significant_delimiters, delimiter)) {
		delimiter = '*';
	}
	char *end;
	if (delimiter == '*') {
		const char *toks = "+- \t"; // Valid delimiters in this scenario
		for (size_t k = 0; k < strlen(toks); ++k) {
			end = code_strchr(code + j, toks[k]);
			if (end) break;
		}
	} else {
		end = code_strchr(code + j, delimiter);
		if (end == NULL && delimiter == ')') {
			end = code_strchr(code + j, ']');
		}
	}
	if (end == NULL) {
		return NULL;
	}
	res = malloc(end - (code + j) + 1);
	strncpy(res, code + j, end - (code + j));
	res[end - (code + j)] = '\0';
	int _;
	res = strip_whitespace(res, &_);
	--*i;
	return res;
}

void match_free(instruction_match_t *match) {
	for (int i = 0; i < match->immediate_values->length; i += 1) {
		immediate_ref_t *ref = (immediate_ref_t*)match->immediate_values->items[i];
		free(ref->value_provided);
		free(ref);
	}
	list_free(match->immediate_values);
	free_flat_list(match->operands);
	free(match);
}

instruction_match_t *try_match(instruction_set_t *set, instruction_t *inst, const char *str) {
	instruction_match_t *result = malloc(sizeof(instruction_match_t));
	result->operands = create_list();
	result->immediate_values = create_list();
	result->instruction = inst;

	int i, j;
	int whitespace_met = 0;
	int match = 1;
	for (i = 0, j = 0; inst->match[i] && str[j]; ++i, ++j) {
		if (inst->match[i] == '_') /* Required whitespace */ {
			if (whitespace_met && isspace(str[j])) {
				/* Current character is whitespace and we have met the whitespace requirement */
				i--;
			} else if (whitespace_met && !isspace(str[j])) {
				/* Current character is not whitespace and we have met the whitespace requirement */
				j--;
				whitespace_met = 0;
			} else {
				/* We have not met the required whitespace yet */
				if (!isspace(str[j])) {
					/* We have not met the required whitespace, and this is not whitespace */
					match = 0;
					break;
				} else {
					/* We have not met the required whitespace, and this is whitespace */
					i--;
					whitespace_met = 1;
				}
			}
		} else if (inst->match[i] == '-') /* Optional whitespace */ {
			if (isspace(str[j])) {
				i--;
			} else {
				j--;
			}
		} else if (inst->match[i] == '%' || inst->match[i] == '^' || inst->match[i] == '&') /* Immediate value */ {
			char type = inst->match[i];
			char key = inst->match[++i];
			if (type != '&') {
				while (inst->match[++i] != '>') { }
				++i;
			} else {
				++i;
			}
			char *value = get_operand_string(inst, &i, str, j);
			if (value == NULL) {
				match = 0;
				break;
			}
			j += strlen(value) - 1;
			immediate_ref_t *ref = malloc(sizeof(immediate_ref_t));
			ref->key = key;
			ref->value_provided = value;
			list_add(result->immediate_values, ref);
			i--;
		} else if (inst->match[i] == '@') /* Operand */ {
			char key = inst->match[++i]; i += 2;
			char *start = inst->match + i;
			int g_len = 0;
			while (inst->match[i] != '>') {
				g_len++; i++;
			}
			++i;
			char *group_name = malloc(g_len + 1);
			strncpy(group_name, start, g_len);
			group_name[g_len] = 0;
			operand_group_t *group = find_operand_group(set, group_name);
			free(group_name);
			if (group == NULL) {
				match = 0;
				break;
			}
			char *value = get_operand_string(inst, &i, str, j);
			if (value == NULL) {
				match = 0;
				break;
			}
			operand_t *op = find_operand(group, value);
			if (op == NULL) {
				match = 0;
				free(value);
				break;
			}
			instruction_operand_t *inst_op = find_instruction_operand(inst, key);
			operand_ref_t *ref = malloc(sizeof(operand_ref_t));
			ref->shift = inst_op->shift;
			ref->op = op;
			list_add(result->operands, ref);
			j += strlen(value) - 1;
			free(value);
			i--;
		} else {
			if (inst->match[i] == '(' && str[j] == '[') {
				scas_log(L_DEBUG, "Permitting [ in place of (");
				continue;
			}
			if (inst->match[i] == ')' && str[j] == ']') {
				scas_log(L_DEBUG, "Permitting ] in place of )");
				continue;
			}
			if (toupper(inst->match[i]) != toupper(str[j])) {
				match = 0;
				break;
			}
		}
	}
	if (str[j] || inst->match[i] || !match) {
		/* Not a match, clean up */
		free_flat_list(result->operands);
		for (i = 0; i < result->immediate_values->length; ++i) {
			immediate_ref_t *ref = result->immediate_values->items[i];
			free(ref->value_provided);
			free(ref);
		}
		list_free(result->immediate_values);
		free(result);
		return NULL;
	}
	return result;
}

instruction_match_t *match_instruction(instruction_set_t *set, const char *str) {
	int i;
	for (i = 0; i < set->instructions->length; ++i) {
		instruction_t *inst = set->instructions->items[i];
		instruction_match_t *match = try_match(set, inst, str);
		if (match) {
			return match;
		}
	}
	return NULL;
}
