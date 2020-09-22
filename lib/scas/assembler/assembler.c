#include "assembler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "privatize.h"
#include "list.h"
#include "log.h"
#include "runtime.h"
#include "objects.h"
#include "readline.h"
#include "stringop.h"
#include "instructions.h"
#include "match.h"
#include "errors.h"
#include "expression.h"
#include "directives.h"
#ifndef _WIN32
#include <strings.h>
#endif

#define ERROR(ERROR_CODE, COLUMN, ...) add_error(state->errors, ERROR_CODE, \
		*(int*)stack_peek(state->line_number_stack), \
		state->line, COLUMN, stack_peek(state->file_name_stack), ##__VA_ARGS__);

struct assembler_state state;

int log10_u64(uint64_t i) {
	int log = 1;
	while (i >= 10) {
		i /= 10;
		log++;
	}
	return log;
}

void transform_local_labels(tokenized_expression_t *expression, const char *last_global_label) {
	for (unsigned int i = 0; i < expression->tokens->length; ++i) {
		expression_token_t *token = expression->tokens->items[i];
		if (token->type == SYMBOL) {
			if (isdigit(token->symbol[0]) 
					&& token->symbol[strlen(token->symbol) - 1] == '$') {
				/* ASxxxx local label */
				const char *fmtstring = "_ASxxxx_%s@%s";
				char *temp = malloc((strlen(fmtstring) - 4) 
						+ strlen(token->symbol)
						+ strlen(last_global_label) 
						+ 1);
				sprintf(temp, fmtstring, token->symbol, last_global_label);
				temp[(strlen(fmtstring) - 4) 
						+ strlen(token->symbol)
						+ strlen(last_global_label)] = '\0';
				scas_log(L_DEBUG, "Transformed ASxxxx label from %s to %s", token->symbol, temp);
				free(token->symbol);
				token->symbol = temp;
			} else if (token->symbol[0] == '.') {
				const char *fmtstring = "%s@%s";
				char *temp = malloc((strlen(fmtstring) - 4) 
						+ (strlen(token->symbol) - 1)
						+ strlen(last_global_label) 
						+ 1);
				sprintf(temp, fmtstring, token->symbol + 1, last_global_label);
				temp[(strlen(fmtstring) - 4) 
						+ (strlen(token->symbol) - 1)
						+ strlen(last_global_label)] = '\0';
				scas_log(L_DEBUG, "Transformed local label from %s to %s", token->symbol, temp);
				free(token->symbol);
				token->symbol = temp;
			}
		}
	}
}

void transform_relative_labels(tokenized_expression_t *expression, int last_relative_label, const char *const file_name) {
	for(unsigned int i = 0; i < expression->tokens->length; i++) {
		expression_token_t *token = expression->tokens->items[i];
		if (token->type != SYMBOL || strcmp(token->symbol, "_")) {
			continue;
		}

		unsigned int j = i;
		int offset = get_relative_label_offset(expression, &j);
		int relative_label = offset + last_relative_label;

		const char *fmtstring = "relative:%d@%s";
		int len = log10_u64(relative_label);
		const int size = strlen(fmtstring) - 2 + len - 2 + strlen(file_name) + 1;
		free(token->symbol);
		token->symbol = malloc(size);
		if (snprintf(token->symbol, size, fmtstring, relative_label, file_name) != size - 1) {
			scas_log(L_ERROR, "UNREACHABLE");
			exit(1);
		}

		scas_log(L_DEBUG, "Transformed relative label with offset %d to %s, %d - %d", offset, token->symbol, i, j);
		while (i + 1 < expression->tokens->length) {
			expression_token_t *next = expression->tokens->items[i + 1];
			if (next->type == OPERATOR && next->operator == 1) {
				free_expression_token(next);
				list_del(expression->tokens, i + 1);
			}
			else {
				break;
			}
		}
	}
}

int try_empty_line(struct assembler_state *state, char **line) {
	(void)state;
	return strlen(*line) == 0;
}

char *extract_macro_parameters(char *str, list_t *params) {
	int i, j, _;
	int in_character = 0, in_string = 0, nested = 0;
	for (i = 0, j = 0; str[i]; ++i) {
		if (str[i] == '"' && !in_character) {
			in_string = !in_string;
		} else if (str[i] == '\'' && !in_string) {
			in_character = !in_character;
		} else if (!in_character && !in_string) {
			if (str[i] == ',') {
				char *param = malloc(i - j + 1);
				strncpy(param, str + j, i - j);
				param[i - j] = '\0';
				param = strip_whitespace(param, &_);
				list_add(params, param);
				j = i + 1;
			} else if (str[i] == '(') {
				++nested;
			} else if (str[i] == ')') {
				if (nested) {
					--nested;
				} else {
					break;
				}
			}
		}
	}
	if (!str[i]) {
		return 0;
	} else {
		char *param = malloc(i - j + 1);
		strncpy(param, str + j, i - j);
		param[i - j] = '\0';
		param = strip_whitespace(param, &_);
		list_add(params, param);
		return str + i + 1;
	}
}

void substitute_parameter(char **line, char *param, char *value) {
	char *match = *line;
	while ((match = strstr(match, param))) {
		char *new = malloc(
			(strlen(*line) - strlen(param)) +
			strlen(value) + 1);
		int n = match - *line;
		strncpy(new, *line, n);
		new[match - *line] = '\0';
		strcat(new, value);
		strcat(new, match + strlen(param));
		free(*line);
		*line = new;
		match = *line + n + strlen(value);
	}
}

int try_expand_macro(struct assembler_state *state, char **line) {
	if (strstr(*line, "macro") == *line + 1) { // Cannot expand macros while defining them
		return 0;
	}
	if (strstr(*line, "undefine") == *line + 1) { // Should not expand while removing
		return 0;
	}
	if (strstr(*line, "ifdef") == *line + 1) { // Should not expand when testing for existence
		return 0;
	}
	// Shouldn't expand macros in equates
	if ((**line == '.' || **line == '#')) {
		if (code_strstr(*line, ".equ") || code_strchr(*line, '=')) {
			return 0;
		}
	}
	for (unsigned int i = 0; i < state->macros->length; i++) {
		macro_t *macro = state->macros->items[i];
		if (macro == state->most_recent_macro) {
			continue;
		}
		int name_length = strlen(macro->name);
		char *match = strstr(*line, macro->name);
		if (match == NULL) {
			continue;
		}
		scas_log(L_DEBUG, "Expanding macro '%s' at %s:%d:%d",
			macro->name,
			(char *)stack_peek(state->file_name_stack),
			*(int *)stack_peek(state->line_number_stack),
			match - *line);

		list_t *userparams = create_list();
		char *endmatch;
		if (match[name_length] == '(') {
			if (!(endmatch = extract_macro_parameters(match + name_length + 1, userparams))) {
				free_flat_list(userparams);
				scas_log(L_DEBUG, "Not a perfect match, skipping");
				continue;
			}
			if (userparams->length != macro->parameters->length) {
				scas_log(L_DEBUG, "Not a perfect match, skipping");
				free_flat_list(userparams);
				continue;
			}
		} else {
			if (macro->parameters->length != 0) {
				scas_log(L_DEBUG, "Not a perfect match, skipping");
				free_flat_list(userparams);
				continue;
			}
			endmatch = match + name_length;
		}

		list_t *newlines = create_list();
		for (unsigned int j = 0; j < macro->macro_lines->length; ++j) {
			char *mline = macro->macro_lines->items[j];
			scas_log(L_DEBUG, "Inserting line '%s'", mline);
			if (j == 0) {
				/* Replacing **line */
				char *newline = malloc(
					(strlen(*line) - (endmatch - match)) +
					strlen(mline) + 1);
				strncpy(newline, *line, match - *line);
				newline[match - *line] = '\0';
				strcat(newline, mline);
				strcat(newline, endmatch);
				list_add(newlines, newline);
			} else {
				/* Adding to extra_lines */
				char *newline = malloc(strlen(mline) + 1);
				strcpy(newline, mline);
				list_add(newlines, newline);
			}
		}

		for (unsigned int j = 0; j < newlines->length; ++j) {
			for (unsigned int k = 0; k < macro->parameters->length; ++k) {
				char *p = macro->parameters->items[k];
				if (!strcmp(p, "")) {
					scas_log(L_ERROR, "Attempt to insert '%s' where no parameter is expected...", (char*)userparams->items[k]);
					break;
				}
				scas_log(L_DEBUG, "Replacing '%s' with '%s' for line '%s'", 
					p, (char *)userparams->items[k], (char *)newlines->items[j]);
				substitute_parameter((char **)&newlines->items[j], p, (char *)userparams->items[k]);
			}
		}

		/* Replace **line with first expanded line... */
		free(*line);
		*line = newlines->items[0];
		list_del(newlines, 0);
		/* ...and add additional lines to extra_lines */
		for (unsigned int j = 0; j < newlines->length; ++j) {
			list_add(state->extra_lines, newlines->items[j]);
		}
		list_free(newlines);
		free_flat_list(userparams);

		if (state->auto_source_maps) {
			add_source_map((source_map_t *)stack_peek(state->source_map_stack),
				*(int *)stack_peek(state->line_number_stack), state->line,
				state->current_area->data_length, 0);
		}
		state->expanding_macro = true;

		state->most_recent_macro = macro;
		return -1;
	}
	return 0;
}

int try_parse_inside_macro(struct assembler_state *state, char **line) {
	if (!state->current_macro) {
		return 0;
	}

	if ((**line == '.' || **line == '#') && strcmp((*line) + 1, "endmacro") == 0) {
		list_add(state->macros, state->current_macro);
		scas_log(L_DEBUG, "Added macro '%s', with %d parameters", state->current_macro->name, state->current_macro->parameters->length);
		state->current_macro = 0;
		return 1;
	}

	char *new_line = malloc(strlen(*line) + 1);
	strcpy(new_line, *line);
	list_add(state->current_macro->macro_lines, new_line);
	return 1;
}

int try_add_label(struct assembler_state *state, char **line) {
	int i;
	for (i = 0; (*line)[i] && (*line)[i] != ':'; ++i) {
		int isvalid = isalnum((*line)[i]) || (*line)[i] == '_' || (*line)[i] == '$' ||
			(i == 0 && (*line)[i] == '.');
		if (!isvalid) {
			return 0;
		}
	}
	if ((*line)[i] != ':') {
		return 0;
	}
	/* Check for illegal label names */
	if (i == 0) {
		scas_log(L_ERROR, "Illegal (Unnamed label)");
		return 0;
	}
	if (strncmp(".", *line, i) == 0) {
		scas_log(L_ERROR, "Illegal (Unnamed local label)");
		return 0;
	}
	if (strncmp("$", *line, i) == 0) {
		// Don't allow "$" alone, it's a special case
		scas_log(L_ERROR, "Illegal ($)");
		return 0;
	}
	if (isdigit(**line) && (*line)[i - 1] != '$') {
		// Don't allow symbols to start with a number unless they're ASxxxx local labels
		return 0;
	}

	/* Add label */
	symbol_t *sym = malloc(sizeof(symbol_t));
	sym->exported = !scas_runtime.options.explicit_export;
	sym->type = SYMBOL_LABEL;
	sym->value = state->PC + scas_runtime.options.origin;
	sym->defined_address = state->current_area->data_length;

	if (isdigit(**line) && (*line)[i - 1] == '$') {
		/* ASxxxx local label, give it a better name */
		char *temp = malloc(i + 1);
		strncpy(temp, *line, i);
		temp[i] = '\0';

		const char *fmtstring = "_ASxxxx_%s@%s";
		sym->name = malloc((strlen(fmtstring) - 4) 
				+ i
				+ strlen(state->last_global_label) 
				+ 1);
		sprintf(sym->name, fmtstring, temp, state->last_global_label);
		sym->name[(strlen(fmtstring) - 4) 
				+ i
				+ strlen(state->last_global_label)] = '\0';
		scas_log(L_DEBUG, "Adding ASxxxx local label %s", sym->name);
		free(temp);
	} else if (strncmp("_", *line, i) == 0) {
		const char *file_name = stack_peek(state->file_name_stack);
		const char *fmtstring = "relative:%d@%s";
		int len = log10_u64(state->last_relative_label);
		const int size = strlen(fmtstring) - 2 + len - 2 + strlen(file_name) + 1;
		sym->name = malloc(size);
		if (!sym->name) {
			scas_log(L_ERROR, "OOM");
			exit(1);
		}
		if (snprintf(sym->name, size, fmtstring, state->last_relative_label++, file_name) != size - 1) {
			scas_log(L_ERROR, "Unreachable.");
			exit(1);
		}
		scas_log(L_DEBUG, "Adding relative label %s", sym->name);
		sym->exported = 0; /* Dont export anonymous labels */
	} else if (**line == '.') {
		char *temp = malloc(i + 1);
		strncpy(temp, *line, i);
		temp[i] = '\0';

		const char *fmtstring = "%s@%s";
		sym->name = malloc((strlen(fmtstring) - 4) 
				+ (i - 1)
				+ strlen(state->last_global_label) 
				+ 1);
		sprintf(sym->name, fmtstring, temp + 1, state->last_global_label);
		sym->name[(strlen(fmtstring) - 4) 
				+ (i - 1)
				+ strlen(state->last_global_label)] = '\0';
		scas_log(L_DEBUG, "Adding local label %s", sym->name);
		free(temp);
	} else {
		sym->name = malloc(i + 1);
		strncpy(sym->name, *line, i);
		sym->name[i] = '\0';
		state->last_global_label = sym->name;
	}
	list_add(state->current_area->symbols, sym);
	list_add(state->object->imports, sym->name);
	scas_log(L_DEBUG, "Added label '%s' (pre-linking value 0x%08X) from %s:%d", sym->name, sym->value,
			(char *)stack_peek(state->file_name_stack), *(int *)stack_peek(state->line_number_stack));
	/* Modify this line so that processing may continue */
	if ((*line)[i + 1] == ':') {
		/* TODO: Export this, it's an ASxxxx global label */
		++i;
	}
	memmove(*line, *line + i + 1, strlen(*line + i));
	int _;
	*line = strip_whitespace(*line, &_);
	return strlen(*line) == 0;
}

int try_match_instruction(struct assembler_state *state, char **_line) {
	char *line = *_line;
	instruction_match_t *match = match_instruction(state->instruction_set, line);
	if (match == NULL) {
		ERROR(ERROR_INVALID_INSTRUCTION, state->column, line);
		return 0;
	} else {
		scas_log(L_DEBUG, "Matched string '%s' to instruction '%s'", line, match->instruction->match);
		scas_log_indent();
		uint64_t instruction = match->instruction->value;
		for (unsigned int i = 0; i < match->operands->length; ++i) {
			operand_ref_t *ref = match->operands->items[i];
			scas_log(L_DEBUG, "Using operand '%s'", ref->op->match);
			instruction |= ref->op->value << (match->instruction->width - ref->shift - ref->op->width);
		}
		for (unsigned int i = 0; i < match->immediate_values->length; ++i) {
			immediate_ref_t *ref = match->immediate_values->items[i];
			immediate_t *imm = find_instruction_immediate(match->instruction, ref->key);

			int error;
			char *symbol;
			uint64_t result;
			tokenized_expression_t *expression = parse_expression(ref->value_provided);
			if (expression == NULL) {
				add_error(state->errors, ERROR_INVALID_SYNTAX,
					*(int*)stack_peek(state->line_number_stack),
					state->line, state->column, stack_peek(state->file_name_stack));
			} else {
				transform_local_labels(expression, state->last_global_label);
				const char *file_name = stack_peek(state->file_name_stack);
				transform_relative_labels(expression, state->last_relative_label, file_name);
				symbol_t sym_pc = {
					.type = SYMBOL_LABEL,
					.value = state->PC,
					.name = "$"
				};
				list_add(state->equates, &sym_pc);
				result = evaluate_expression(expression, state->equates, &error, &symbol);
				state->equates->length -= 1;
				if (error == EXPRESSION_BAD_SYMBOL) {
					if (scas_runtime.options.explicit_import && strcmp(symbol, "$") != 0) {
						unresolved_symbol_t *unresolved_sym = malloc(sizeof(unresolved_symbol_t));
						unresolved_sym->name = strdup(symbol);
						unresolved_sym->column = state->column;
						unresolved_sym->line_number = *(int*)stack_peek(state->line_number_stack);
						unresolved_sym->line = strdup(state->line);
						unresolved_sym->file_name = strdup(stack_peek(state->file_name_stack));
						list_add(state->object->unresolved, unresolved_sym);
					}
					scas_log(L_DEBUG, "Postponing evaluation of '%s' to linker (type %d)", ref->value_provided, imm->type);
					late_immediate_t *late_imm = malloc(sizeof(late_immediate_t));
					late_imm->instruction_address = state->current_area->data_length;
					late_imm->base_address = state->PC + (match->instruction->width / 8);
					late_imm->address = state->current_area->data_length + (imm->shift / 8);
					late_imm->width = imm->width;
					late_imm->type = imm->type;
					late_imm->expression = expression;
					list_add(state->current_area->late_immediates, late_imm);
				} else {
					free_expression(expression);
					if (imm->type == IMM_TYPE_RELATIVE) {
						result = result - ((state->PC + scas_runtime.options.origin)
							+ (match->instruction->width / 8));
					} else if (imm->type == IMM_TYPE_RESTART) {
						if ((result & ~0x07) != result || result > 0x38) {
							/* We get an ERROR_VALUE_TRUNCATED if we just let it proceed */
						} else {
							result >>= 3;
						}
					}
					scas_log(L_DEBUG, "Resolved '%s' early with result 0x%08X", ref->value_provided, result);
					uint64_t mask = 1;
					int shift = imm->width;
					while (--shift) {
						mask <<= 1;
						mask |= 1;
					}
					if ((result & mask) != result && ~result >> imm->width) {
						add_error(state->errors, ERROR_VALUE_TRUNCATED,
							*(int*)stack_peek(state->line_number_stack),
							state->line, state->column, stack_peek(state->file_name_stack));
					} else {
						result = result & mask;
						int bits = imm->width;
						while (bits > 0) {
							bits -= 8;
							if (bits < 0) {
								bits = 0;
							}
							instruction |= (result & 0xFF) << (match->instruction->width - imm->shift - imm->width) << bits;
							result >>= 8;
						}
					}
				}
			}
		}
		unsigned int bytes_width = match->instruction->width / 8;
		for (unsigned int i = 0; i < bytes_width; ++i) {
			state->instruction_buffer[bytes_width - i - 1] = instruction & 0xFF;
			instruction >>= 8;
		}
		/* Add completed instruction */
		if (!state->expanding_macro && state->auto_source_maps) {
			add_source_map((source_map_t *)stack_peek(state->source_map_stack),
				*(int *)stack_peek(state->line_number_stack), state->line,
				state->current_area->data_length, bytes_width);
		}
		append_to_area(state->current_area, state->instruction_buffer, bytes_width);
		state->PC += bytes_width;
		scas_log_deindent();
		match_free(match);
	}
	return 1;
}

char *split_line(struct assembler_state *state, char *line) {
	int i, j, _;
	for (i = 0, j = 0; line[i]; ++i) {
		if (line[i] == '\\') {
			char *part = malloc(i - j + 1);
			strncpy(part, line + j, i - j);
			part[i - j] = '\0';
			part = strip_whitespace(part, &_);
			list_add(state->extra_lines, part);
			j = i + 1;
		}
	}
	char *part = malloc(i - j + 1);
	strncpy(part, line + j, i - j);
	part[i - j] = '\0';
	part = strip_whitespace(part, &_);
	list_add(state->extra_lines, part);
	free(line);
	part = state->extra_lines->items[0];
	list_del(state->extra_lines, 0);
	return part;
}

int try_split_line(struct assembler_state *state, char **line) {
	/* If the line starts with any of these things, don't split it */
	const char *blacklist[] = {
		"#define",
		".define"
	};
	for (size_t i = 0; i < sizeof(blacklist) / sizeof(char *); ++i) {
		if (code_strstr(*line, blacklist[i]) == *line) {
			return 0;
		}
	}
	if (code_strchr(*line, '\\')) {
		*line = split_line(state, *line);
	}
	return 0;
}

void macro_free(macro_t *macro) {
	free_flat_list(macro->parameters);
	free_flat_list(macro->macro_lines);
	free(macro->name);
	free(macro);
}

object_t *assemble(FILE *file, const char *file_name, assembler_settings_t *settings) {
	struct assembler_state state = {
		.object = create_object(),
		.current_area = create_area("_CODE"),
		.source_map_stack = create_stack(),

		.file_stack = create_stack(),
		.file_name_stack = create_stack(),
		.line_number_stack = create_stack(),
		.errors = settings->errors,
		.warnings = settings->warnings,

		.extra_lines = create_list(),
		.expanding_macro = false,
		.line = "",
		.column = 0,

		.instruction_set = settings->set,
		.instruction_buffer = malloc(64 / 8),
		.if_stack = create_stack(),
		.equates = create_list(),
		.macros = create_list(),
		.current_macro = 0,
		.nolist = 0,
		.PC = 0,
		.last_global_label = "@start",
		.settings = settings,

		.most_recent_macro = NULL,

		.auto_source_maps = true,

		.last_relative_label = 0
	};
	list_cat(state.macros, settings->macros);
	int *ln = malloc(sizeof(int));
	*ln = 0;
	char *name = strdup(file_name);
	stack_push(state.file_name_stack, name);
	stack_push(state.line_number_stack, ln);
	stack_push(state.file_stack, file);
	stack_push(state.source_map_stack, create_source_map(state.current_area, file_name));
	list_add(state.object->areas, state.current_area);

	int(*const line_ops[])(struct assembler_state *, char **) = {
		try_empty_line,
		try_parse_inside_macro,
		try_split_line,
		try_add_label,
		try_expand_macro,
		try_handle_directive,
		try_match_instruction,
	};
	int(*const nolist_line_ops[])(struct assembler_state *, char **) = {
		try_empty_line,
		try_handle_directive,
	};

	char *line;
	FILE *cur;
	while ((cur = stack_peek(state.file_stack))) {
		if (!feof(cur) || state.extra_lines->length) {
			if (state.extra_lines->length == 0) {
				++*(int*)stack_peek(state.line_number_stack);
				line = read_line(cur);
				state.most_recent_macro = NULL;
				if (state.expanding_macro) {
					state.expanding_macro = false;
					source_map_t *map = stack_peek(state.source_map_stack);
					source_map_entry_t *entry = map->entries->items[map->entries->length - 1];
					entry->length = state.PC - entry->address;
					if (entry->length == 0) {
						free(entry->source_code);
						free(entry);
						list_del(map->entries, map->entries->length - 1);
					}
				}
			} else {
				line = state.extra_lines->items[0];
				list_del(state.extra_lines, 0);
			}
			state.line = malloc(strlen(line) + 1);
			strcpy(state.line, line);
			line = strip_comments(line);
			line = strip_whitespace(line, &state.column);
			int l = sizeof(line_ops) / sizeof(void*);
			if (state.nolist || (state.if_stack->length && !*(int*)stack_peek(state.if_stack))) {
				l = sizeof(nolist_line_ops) / sizeof(void*);
			}
			scas_log(L_DEBUG, "Assembler handling line '%s'", line);
			for (int i = 0; i < l; ++i) {
				int res;
				if (state.nolist || (state.if_stack->length && !*(int*)stack_peek(state.if_stack))) {
					res = nolist_line_ops[i](&state, &line);
				} else {
					res = line_ops[i](&state, &line);
				}
				if (res == 1) {
					break;
				} else if (res == -1) {
					// Start over
					i = 0;
				}
			}
			free(state.line);
			free(line);
		} else {
			free(stack_pop(state.file_name_stack));
			free(stack_pop(state.line_number_stack));
			if (cur != file) {
				fclose(cur);
				stack_pop(state.file_stack);
				stack_pop(state.source_map_stack);
				scas_log(L_INFO, "Returning to file '%s'",
					(char *)stack_peek(state.file_name_stack));
			} else {
				for (unsigned int i = 0; i < state.object->unresolved->length; i++) {
					unresolved_symbol_t *sym = (unresolved_symbol_t*)
						state.object->unresolved->items[i];
					bool imported = false;
					for (unsigned int j = 0; j < state.object->imports->length; j++) {
						if (strcmp(sym->name, state.object->imports->items[j]) == 0) {
							imported = true;
							break;
						}
					}
					if (!imported) {
						add_error(state.errors, ERROR_UNKNOWN_SYMBOL,
							sym->line_number, sym->line, sym->column,
							sym->file_name, sym->name);
					}
				}
				break;
			}
		}
	}

	while (state.if_stack->length != 0) {
		free(stack_pop(state.if_stack));
	}
	stack_free(state.if_stack);
	stack_free(state.file_stack);
	stack_free(state.file_name_stack);
	stack_free(state.line_number_stack);
	list_free(state.extra_lines);
	for (unsigned int i = 0; i < state.macros->length; i += 1) {
		macro_t *macro = state.macros->items[i];
		macro_free(macro);
	}
	list_free(state.macros);
	// Equates are also added to an area's symbols list, so they're cleaned up there
	list_free(state.equates);
	stack_free(state.source_map_stack);
	free(state.instruction_buffer);

	privatize_symbols(state.object);

	return state.object;
}
