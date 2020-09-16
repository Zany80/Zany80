#include "directives.h"
#include "assembler.h"
#include "errors.h"
#include "functions.h"
#include "expression.h"
#include "stringop.h"
#include "objects.h"
#include "runtime.h"
#include "list.h"
#include "log.h"
#include "format.h"
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef _WIN32
#include <strings.h>
#endif

#define ERROR(ERROR_CODE, COLUMN, ...) add_error(state->errors, ERROR_CODE, \
		*(int*)stack_peek(state->line_number_stack), \
		state->line, COLUMN, stack_peek(state->file_name_stack) , ##__VA_ARGS__);

#define ERROR_NO_ARG(ERROR_CODE, COLUMN) add_error(state->errors, ERROR_CODE, \
		*(int*)stack_peek(state->line_number_stack), \
		state->line, COLUMN, stack_peek(state->file_name_stack));

#define WARN(WARN_CODE, COLUMN, ...) add_warning(state->warnings, WARN_CODE, \
		*(int*)stack_peek(state->line_number_stack), \
		state->line, COLUMN, stack_peek(state->file_name_stack) , ##__VA_ARGS__);

#define MAP_SOURCE(LENGTH) if (!state->expanding_macro && state->auto_source_maps) { \
			add_source_map((source_map_t *)stack_peek(state->source_map_stack), \
			*(int*)stack_peek(state->line_number_stack), state->line, \
			state->current_area->data_length, LENGTH); \
		}

enum {
	DELIM_COMMAS = 1,
	DELIM_WHITESPACE = 2,
};

const char *delimiters_list[] = {
	"",		// 0
	",",	// DELIM_COMMAS
	" \t",	// DELIM_WHITESPACE
	", \t", // DELIM_COMMAS | DELIM_WHITESPACE
};

struct directive {
	char *match;
	int(*function)(struct assembler_state *state, char **argv, int argc);
	int delimiter;
};

char *join_args(char **argv, int argc) {
	int len = 0, i;
	for (i = 0; i < argc; ++i) {
		len += strlen(argv[i]) + 1;
	}
	char *res = malloc(len);
	len = 0;
	for (i = 0; i < argc; ++i) {
		strcpy(res + len, argv[i]);
		len += strlen(argv[i]);
		res[len++] = ' ';
	}
	res[len - 1] = '\0';
	return res;
}

int handle_nop(struct assembler_state *state, char **argv, int argc) {
	(void)state;
	(void)argv;
	(void)argc;
	return 1;
}

int handle_asciiz(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "asciiz expects 1+ arguments");
		return 1;
	}
	int i;
	for (i = 0; i < argc; ++i) {
		int len = strlen(argv[i]);
		if (argv[i][0] != '"' || argv[i][len - 1] != '"') {
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated string");
			return 1;
		}
		len = unescape_string(argv[i] + 1);
		MAP_SOURCE(len + 1);
		argv[i][len] = 0;
		append_to_area(state->current_area, (unsigned char*)(argv[i] + 1), len /* Includes the null terminator */);
		state->PC += len;
	}
	return 1;
}

int handle_asciip(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "asciip expects 1+ arguments");
		return 1;
	}
	int i;
	for (i = 0; i < argc; ++i) {
		int len = strlen(argv[i]);
		if (argv[i][0] != '"' || argv[i][len - 1] != '"') {
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated string");
			return 1;
		}
		argv[i][len - 1] = '\0';
		len -= 2;
		len = unescape_string(argv[i] + 1);
		uint8_t _len = len;
		if (_len != len) {
			/* Would it be obvious that this is because the string is too long? */
			ERROR_NO_ARG(ERROR_VALUE_TRUNCATED, state->column);
		}
		MAP_SOURCE(len + 1);
		append_to_area(state->current_area, &_len, sizeof(uint8_t));
		append_to_area(state->current_area, (unsigned char*)(argv[i] + 1), len);
		state->PC += len + 1;
	}
	return 1;
}

int handle_block(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "block expects 1 argument");
		return 1;
	}
	tokenized_expression_t *expression = parse_expression(argv[0]);
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} 

	symbol_t sym_pc = {
		.type = SYMBOL_LABEL,
		.value = state->PC,
		.name = "$"
	};

	list_add(state->equates, &sym_pc);
	int error;
	char *symbol;
	uint16_t result = evaluate_expression(expression, state->equates, &error, &symbol);
	list_del(state->equates, state->equates->length - 1); // Remove $

	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
		free_expression(expression);
		return 1;
	} 

	uint8_t *buffer = calloc(256, sizeof(uint8_t));
	MAP_SOURCE(result);
	while (result) {
		append_to_area(state->current_area, buffer, result > 256 ? 256 : result);
		if (result > 256) {
			result -= 256;
			state->PC += 256;
		} else {
			state->PC += result;
			result = 0;
		}
	}
	free(buffer);
	free_expression(expression);
	return 1;
}

int handle_bndry(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "bndry expects 1 argument");
		return 1;
	}

	tokenized_expression_t *expression = parse_expression(argv[0]);
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} 

	int error;
	char *symbol;
	uint16_t result = evaluate_expression(expression, state->equates, &error, &symbol);
	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
		return 1;
	}
	if (state->PC % result != 0) {
		uint8_t *buf = calloc(1024, sizeof(uint8_t));
		int len = state->PC % result;
		MAP_SOURCE(len);
		while (len) {
			append_to_area(state->current_area, buf, len > 256 ? 256 : len);
			if (len > 256) {
				len -= 256;
				state->PC += 256;
			} else {
				state->PC += len;
				len = 0;
			}
		}
	}
	return 1;
}

int handle_db(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "db expects 1+ arguments");
		return 1;
	}
	uint64_t olen = 0;
	uint64_t dlen = state->current_area->data_length;
	int i;
	for (i = 0; i < argc; ++i) {
		int len = strlen(argv[i]);
		if (argv[i][0] == '"' && argv[i][len - 1] == '"') {
			/* TODO: Do we need to do anything fancy wrt encoding? */
			argv[i][len - 1] = '\0';
			len -= 2;
			len = unescape_string(argv[i] + 1);
			olen += len;
			append_to_area(state->current_area, (unsigned char*)(argv[i] + 1), len);
			state->PC += len;
		} else {
			tokenized_expression_t *expression = parse_expression(argv[i]);
			bool keep = false;

			if (expression == NULL) {
				ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
			} else {

				int error;
				char *symbol;
				uint64_t result = evaluate_expression(expression, state->equates, &error, &symbol);

				if (error == EXPRESSION_BAD_SYMBOL) {
					if (scas_runtime.options.explicit_import) {
						tokenized_expression_t *changed_expression = malloc(sizeof(tokenized_expression_t));
						memcpy(changed_expression, expression, sizeof(tokenized_expression_t));
						int ignored_error;
						char *fixed_symbol;
						transform_local_labels(changed_expression, state->last_global_label);
						evaluate_expression(expression, state->equates, &ignored_error, &fixed_symbol);
						unresolved_symbol_t *unresolved_sym = malloc(sizeof(unresolved_symbol_t));
						unresolved_sym->name = malloc(strlen(fixed_symbol) + 1);
						strcpy(unresolved_sym->name, fixed_symbol);
						unresolved_sym->column = state->column;
						unresolved_sym->line_number = *(int*)stack_peek(state->line_number_stack);
						unresolved_sym->line = malloc(strlen(state->line) + 1);
						strcpy(unresolved_sym->line, state->line);
						const char *file_name=stack_peek(state->file_name_stack);
						unresolved_sym->file_name = malloc(sizeof(file_name) + 1);
						strcpy(unresolved_sym->file_name, file_name);
						list_add(state->object->unresolved, unresolved_sym);
					}

					scas_log(L_DEBUG, "Postponing evaluation of '%s' to linker", argv[i]);
					late_immediate_t *late_imm = malloc(sizeof(late_immediate_t));
					late_imm->address = state->current_area->data_length;
					late_imm->instruction_address = state->current_area->data_length;
					late_imm->base_address = state->current_area->data_length;
					late_imm->width = 8;
					late_imm->type = IMM_TYPE_ABSOLUTE;
					late_imm->expression = expression;
					keep = true;
					list_add(state->current_area->late_immediates, late_imm);
					*state->instruction_buffer = 0;
				} else if ((result & 0xFF) != result && ~result >> 8) {
					ERROR_NO_ARG(ERROR_VALUE_TRUNCATED, state->column);
				} else {
					*state->instruction_buffer = (uint8_t)result;
				}
			}
			++olen;
			append_to_area(state->current_area, state->instruction_buffer, 1);
			++state->PC;
			if (!keep) {
				free_expression(expression);
			}
		}
	}
	if (!state->expanding_macro) {
		add_source_map((source_map_t *)stack_peek(state->source_map_stack),
			*(int*)stack_peek(state->line_number_stack), state->line, dlen, olen);
	}
	return 1;
}

int handle_dl(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "dl expects 1+ arguments");
		return 1;
	}
	uint64_t olen = 0;
	int i;
	for (i = 0; i < argc; ++i) {
		tokenized_expression_t *expression = parse_expression(argv[i]);

		if (expression == NULL) {
			ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
			return 1;
		}

		int error;
		char *symbol;
		uint64_t result = evaluate_expression(expression, state->equates, &error, &symbol);

		if (error == EXPRESSION_BAD_SYMBOL) {
			if (scas_runtime.options.explicit_import) {
				tokenized_expression_t *changed_expression = malloc(sizeof(tokenized_expression_t));
				memcpy(changed_expression, expression, sizeof(tokenized_expression_t));
				int ignored_error;
				char *fixed_symbol;
				transform_local_labels(changed_expression, state->last_global_label);
				evaluate_expression(expression, state->equates, &ignored_error, &fixed_symbol);
				unresolved_symbol_t *unresolved_sym = malloc(sizeof(unresolved_symbol_t));
				unresolved_sym->name = malloc(strlen(fixed_symbol) + 1);
				strcpy(unresolved_sym->name,fixed_symbol);
				unresolved_sym->column = state->column;
				unresolved_sym->line_number = *(int*)stack_peek(state->line_number_stack);
				unresolved_sym->line = malloc(strlen(state->line) + 1);
				strcpy(unresolved_sym->line, state->line);
				const char *file_name = stack_peek(state->file_name_stack);
				unresolved_sym->file_name = malloc(sizeof(file_name) + 1);
				strcpy(unresolved_sym->file_name, file_name);
				list_add(state->object->unresolved, unresolved_sym);
			}

			scas_log(L_DEBUG, "Postponing evaluation of '%s' to linker", argv[i]);
			late_immediate_t *late_imm = malloc(sizeof(late_immediate_t));
			late_imm->address = state->current_area->data_length;
			late_imm->instruction_address = state->current_area->data_length;
			late_imm->base_address = state->current_area->data_length;
			late_imm->width = 32;
			late_imm->type = IMM_TYPE_ABSOLUTE;
			late_imm->expression = expression;
			list_add(state->current_area->late_immediates, late_imm);
			state->instruction_buffer[0] = 0;
			state->instruction_buffer[1] = 0;
			state->instruction_buffer[2] = 0;
			state->instruction_buffer[3] = 0;
		} else {
			if ((result & 0xFFFFFFFF) != result && ~result >> 32) {
				ERROR_NO_ARG(ERROR_VALUE_TRUNCATED, state->column);
			} else {
				state->instruction_buffer[3] = (uint8_t)((result >> 24) & 0xFF);
				state->instruction_buffer[2] = (uint8_t)((result >> 16) & 0xFF);
				state->instruction_buffer[1] = (uint8_t)((result >> 8) & 0xFF);
				state->instruction_buffer[0] = (uint8_t)(result & 0xFF);
			}
		}
		append_to_area(state->current_area, state->instruction_buffer, 4);
		state->PC += 4;
		olen += 4;
	}
	if (!state->expanding_macro) {
		add_source_map((source_map_t *)stack_peek(state->source_map_stack),
			*(int*)stack_peek(state->line_number_stack), state->line,
			state->current_area->data_length, olen);
	}
	return 1;
}

int handle_define(struct assembler_state *state, char **argv, int argc) {
	/* Basically the same thing as handle_macro, but everything is on 1 line */
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "define expects 1+ arguments");
	}
	char *old = argv[0];
	argv[0] = join_args(argv, argc);
	free(old);
	char *location = strchr(argv[0], '(');
	if (location == argv[0]) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unnamed macro");
		return 1;
	}

	macro_t *define = malloc(sizeof(macro_t));
	define->parameters = create_list();
	define->macro_lines = create_list();
	if (location) {
		int i, _;
		define->name = malloc (location - argv[0] + 1);
		strncpy(define->name, argv[0], location - argv[0] );
		define->name[location - argv[0]] = 0; /* End of String */
		++location;
		char *end = strchr(location, ')');
		if (!end) {
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated parameters");
			return 1;
		}

		char *params = malloc(end - location + 1);
		strncpy(params, location, end - location);
		params[end - location] = 0;
		list_t *parameters = split_string(params, ",");

		for (i = 0; i < parameters->length; i++) {
			char *parameter = parameters->items[i];
			list_add(define->parameters, strip_whitespace(parameter, &_));
		}
		location = end + 1; /* After the parenthesis */
	} else {
		location = argv[0];
		while (*location && !isspace(*location)) ++location;
		char _ = *location;
		*location = '\0';
		define->name = strdup(argv[0]);
		*location = _;
		++location;
	}
	if ((strlen(argv[0]) + 1) == (size_t)(location - argv[0])) { /* End of string? */
		list_add(define->macro_lines, strdup("1")); /* default value is 1 */
	} else {
		while (isspace(*location)) {
			++location;
		}
		list_add(define->macro_lines, strdup(location)); /* Rest of the line */
	}

	list_add(state->macros, define);
	scas_log(L_DEBUG, "Added define \'%s\' with %d parameters", define->name, define->parameters->length);
	return 1;
}

int handle_undef(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "undef expects 1 argument");
		return 1;
	}

	scas_log(L_DEBUG, "Looking for %s", argv[0]);

	int i;
	for (i = 0; i < state->macros->length; i++) {
		macro_t *m = state->macros->items[i];
		scas_log(L_DEBUG, "Found %s", m->name);
		if (strcasecmp(m->name, argv[0]) == 0) {
			scas_log(L_DEBUG, "Undefined macro \'%s\'", m->name);
			macro_free(m);
			list_del(state->macros, i);
			return 1;
		}
	}
	ERROR(ERROR_UNKNOWN_SYMBOL, state->column, argv[0]);
	return 1;
}

int handle_area(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		/* This space intentionally left blank */
	}
	area_t *area = NULL;
	int i;
	for (i = 0; i < state->object->areas->length; ++i) {
		area_t *a = state->object->areas->items[i];
		if (strcasecmp(a->name, argv[0]) == 0) {
			area = a;
			break;
		}
	}
	if (!area) {
		area = create_area(argv[0]);
		list_add(state->object->areas, area);
	}
	state->current_area = area;
	state->PC = area->data_length;
	scas_log(L_INFO, "Switched to area '%s' from directive at %s:%d", area->name,
			(char *)stack_peek(state->file_name_stack), *(int *)stack_peek(state->line_number_stack));
	stack_pop(state->source_map_stack);
	stack_push(state->source_map_stack, create_source_map(state->current_area,
			(char *)stack_peek(state->file_name_stack)));
	return 1;
}

int handle_ascii(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "ascii expects 1+ arguments");
		return 1;
	}
	int i;
	for (i = 0; i < argc; ++i) {
		int len = strlen(argv[i]);
		if (argv[i][0] != '"' || argv[i][len - 1] != '"') {
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated string");
			return 1;
		}
		len = unescape_string(argv[i] + 1) - 1;
		MAP_SOURCE(len);
		append_to_area(state->current_area, (unsigned char*)(argv[i] + 1), len);
		state->PC += len;
	}
	return 1;
}

int handle_dw(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "dw expects 1+ arguments");
		return 1;
	}
	uint64_t olen = 0;
	int i;
	for (i = 0; i < argc; ++i) {
		int error;
		uint64_t result;
		char *symbol;
		tokenized_expression_t *expression = parse_expression(argv[i]);
		bool keep = false;

		if (expression == NULL) {
			ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		} else {
			result = evaluate_expression(expression, state->equates, &error, &symbol);
			transform_local_labels(expression, state->last_global_label);

			if (error == EXPRESSION_BAD_SYMBOL) {
				if (scas_runtime.options.explicit_import) {
					int ignored_error;
					char *fixed_symbol;
					evaluate_expression(expression, state->equates, &ignored_error, &fixed_symbol);
					unresolved_symbol_t *unresolved_sym = malloc(sizeof(unresolved_symbol_t));
					unresolved_sym->name = malloc(strlen(fixed_symbol) + 1);
					strcpy(unresolved_sym->name,fixed_symbol);
					unresolved_sym->column = state->column;
					unresolved_sym->line_number = *(int*)stack_peek(state->line_number_stack);
					unresolved_sym->line = malloc(strlen(state->line) + 1);
					strcpy(unresolved_sym->line, state->line);
					const char *file_name = stack_peek(state->file_name_stack);
					unresolved_sym->file_name = strdup(file_name);
					list_add(state->object->unresolved, unresolved_sym);
				}

				scas_log(L_DEBUG, "Postponing evaluation of '%s' to linker", argv[i]);
				late_immediate_t *late_imm = malloc(sizeof(late_immediate_t));
				late_imm->address = state->current_area->data_length;
				late_imm->instruction_address = state->current_area->data_length;
				late_imm->base_address = state->current_area->data_length;
				late_imm->width = 16;
				late_imm->type = IMM_TYPE_ABSOLUTE;
				late_imm->expression = expression;
				keep = true;
				list_add(state->current_area->late_immediates, late_imm);
				state->instruction_buffer[0] = 0;
				state->instruction_buffer[1] = 0;
			} else {
				if ((result & 0xFFFF) != result && ~result >> 16) {
					ERROR_NO_ARG(ERROR_VALUE_TRUNCATED, state->column);
				} else {
					state->instruction_buffer[1] = (uint8_t)(result >> 8);
					state->instruction_buffer[0] = (uint8_t)(result & 0xFF);
				}
			}
		}
		if (!keep) {
			free_expression(expression);
		}
		append_to_area(state->current_area, state->instruction_buffer, 2);
		state->PC += 2;
		olen += 2;
	}
	if (!state->expanding_macro) {
		add_source_map((source_map_t *)stack_peek(state->source_map_stack),
			*(int*)stack_peek(state->line_number_stack), state->line,
			state->current_area->data_length, olen);
	}
	return 1;
}

struct assembler_state *printf_state;
char **printf_argv;
int printf_argc;

static uintmax_t printf_arg(size_t size) {
	(void)size;
	// TODO: support strings?

	int error;
	uint64_t result;
	char *symbol;

	struct assembler_state *state = printf_state;

	if (!printf_argc) {
		ERROR_NO_ARG(ERROR_INVALID_DIRECTIVE, state->column);
		return 0;
	}

	char *arg = *printf_argv;
	++printf_argv;
	--printf_argc;

	tokenized_expression_t *expression = parse_expression(arg);

	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 0;
	}
	// The only error evaluate_expression can return is EXPRESSION_BAD_SYMBOL
	result = evaluate_expression(expression, state->equates, &error, &symbol);
	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 0;
	}
	return result;
}

static void printf_putc(char c) {
	putchar(c);
}

int handle_echo(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "echo expects 1+ arguments");
		return 1;
	}
	(void)argv;
	return 1;
}

int handle_printf(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "printf expects 1+ arguments");
		return 1;
	}
	int len = strlen(argv[0]);
	if (argv[0][0] != '"' || argv[0][len - 1] != '"') {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated string");
		return 1;
	}
	argv[0][len - 1] = '\0';
	len -= 2;
	printf_argv = argv + 1;
	printf_argc = argc - 1;
	printf_state = state;
	format(printf_putc, printf_arg, argv[0] + 1);
	return 1;
}

int handle_elseif(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "elseif expects 1 argument");
		return 1;
	}
	if (state->if_stack->length == 0) {
		ERROR_NO_ARG(ERROR_TRAILING_END, state->column);
		return 1;
	}
	if (*(int *)stack_peek(state->if_stack)) {
		return 1;
	}
	tokenized_expression_t *expression = parse_expression(argv[0]);
	int error;
	char *symbol;
	uint64_t result;
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else {
		result = evaluate_expression(expression, state->equates, &error, &symbol);
	}
	if (error == EXPRESSION_BAD_SYNTAX) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
		return 1;
	} else {
		*(int *)stack_peek(state->if_stack) = result;
		scas_log(L_DEBUG, "Set if state to %d from elseif directive", result);
	}
	return 1;
}

int handle_else(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "else expects 0 arguments");
		return 1;
	}
	if (state->if_stack->length == 0) {
		ERROR_NO_ARG(ERROR_TRAILING_END, state->column);
		return 1;
	}
	bool nested_false = false;
	for (int i = 0; i < state->if_stack->length - 1; i++) {
		int *conditional = state->if_stack->items[i];
		if (!*conditional) {
		nested_false = true;
		break;
		}
	}
	if (nested_false) {
		scas_log(L_DEBUG, "Ignoring else within falsy if directive");
	}
	else {
		int *top = stack_peek(state->if_stack);
		*top = !*top;
		scas_log(L_DEBUG, "Inverted if state (now %d) from else directive", *top);
	}
	return 1;
}

int handle_end(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "end expects 0 arguments");
		return 1;
	}
	if (state->if_stack->length == 0) {
		/* nop */
		return 1;
	}
	stack_pop(state->if_stack);
	return 1;
}

int handle_endif(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "endif expects 0 arguments");
		return 1;
	}
	if (state->if_stack->length == 0) {
		ERROR_NO_ARG(ERROR_TRAILING_END, state->column);
		return 1;
	}
	free(stack_pop(state->if_stack));
	scas_log(L_DEBUG, "Encountered .endif directive");
	return 1;
}

int handle_equ(struct assembler_state *state, char **argv, int argc) {
	if (argc < 2) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "equ expects 2+ arguments");
		return 1;
	}
	char *args = join_args(argv + 1, argc - 1);
	tokenized_expression_t *expression = parse_expression(args);
	free(args);
	int error;
	char *symbol;
	uint64_t result;
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else {
		transform_local_labels(expression, state->last_global_label);
		int len = state->equates->length;
		for (int i = 0; i < state->current_area->symbols->length; i += 1) {
			list_add(state->equates, state->current_area->symbols->items[i]);
		}
		symbol_t sym_pc = {
			.type = SYMBOL_LABEL,
			.value = state->PC,
			.name = "$"
		};
		list_add(state->equates, &sym_pc);
		result = evaluate_expression(expression, state->equates, &error, &symbol);
		state->equates->length = len;
	}
	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
	} else if (error == EXPRESSION_BAD_SYNTAX) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
	} else {
		symbol_t *sym = malloc(sizeof(symbol_t));
		sym->name = malloc(strlen(argv[0]) + 1);
		strcpy(sym->name, argv[0]);
		sym->type = SYMBOL_EQUATE;
		sym->value = result;
		sym->exported = 0;
		list_add(state->equates, sym);
		list_add(state->current_area->symbols, sym);
		scas_log(L_DEBUG, "Added equate '%s' with value 0x%08X", sym->name, sym->value);
	}
	free_expression(expression);
	return 1;
}

int handle_fill(struct assembler_state *state, char **argv, int argc) {
	if (argc < 1 || argc > 2) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "fill expects one or two arguments");
		return 1;
	}

	uint8_t value = 0;

	tokenized_expression_t *expression = parse_expression(argv[0]);
	
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	}

	symbol_t sym_pc = {
		.type = SYMBOL_LABEL,
		.value = state->PC,
		.name = "$"
	};
	list_add(state->equates, &sym_pc);

	int error;
	char *symbol;
	uint16_t size = evaluate_expression(expression, state->equates, &error, &symbol);
	
	free_expression(expression);
	list_del(state->equates, state->equates->length - 1); // Remove $
	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
	} else {
		if (size == 0) {
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, "Fill requires a non-zero size");
			return 1;
		}
		if (argc == 2) {
			tokenized_expression_t *expression = parse_expression(argv[1]);
			if (expression == NULL) {
				error = EXPRESSION_BAD_SYNTAX;
			} else {
				symbol_t sym_pc = {
					.type = SYMBOL_LABEL,
					.value = state->PC,
					.name = "$"
				};
				list_add(state->equates, &sym_pc);
				uint64_t result = evaluate_expression(expression, state->equates, &error, &symbol);
				free_expression(expression);
				list_del(state->equates, state->equates->length - 1); // Remove $5
				if ((result & 0xFF) != result) {
					ERROR_NO_ARG(ERROR_VALUE_TRUNCATED, state->column);
					return 1;
				}
				value = result & 0xFF;
			}
		}
		uint8_t *buffer = malloc(sizeof(uint8_t) * size);
		memset(buffer, value, size);
		append_to_area(state->current_area, buffer, sizeof(uint8_t) * size);
		free(buffer);
		state->PC += size;
	}
	return 1;
}

int handle_even(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "even expects 0 arguments");
		return 1;
	}
	if (state->PC % 2 != 0) {
		uint8_t pad = 0;
		MAP_SOURCE(1);
		append_to_area(state->current_area, &pad, sizeof(uint8_t));
		++state->PC;
		scas_log(L_DEBUG, "Added byte to pad for .even directive");
	}
	return 1;
}

int handle_function(struct assembler_state *state, char **argv, int argc) {
	if (argc != 3) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "function expects 3 arguments");
		return 1;
	}
	metadata_t *meta = get_area_metadata(state->current_area, "scas.functions");
	if (meta == NULL) {
		meta = malloc(sizeof(metadata_t));
		meta->value = malloc(sizeof(uint32_t));
		meta->value_length = sizeof(uint32_t);
		*(uint32_t *)meta->value = 0;
	}
	list_t *functions = decode_function_metadata(state->current_area, meta->value);
	function_metadata_t *new_function = malloc(sizeof(function_metadata_t));

	new_function->name = malloc(strlen(argv[0]) + 1);
	strcpy(new_function->name, argv[0]);
	new_function->start_symbol = malloc(strlen(argv[1]) + 1);
	strcpy(new_function->start_symbol, argv[1]);
	new_function->end_symbol = malloc(strlen(argv[2]) + 1);
	strcpy(new_function->end_symbol, argv[2]);
	scas_log(L_DEBUG, "Added function declaration %s from %s to %s",
			new_function->name, new_function->start_symbol, new_function->end_symbol);

	list_add(functions, new_function);
	free(meta->value);
	meta->value = encode_function_metadata(functions, &meta->value_length);
	list_free(functions);
	set_area_metadata(state->current_area, "scas.functions", meta->value, meta->value_length);
	return 1;
}

int handle_export(struct assembler_state *state, char **argv, int argc) {
	if (!scas_runtime.options.explicit_export) {
		scas_log(L_INFO, "Implicitly enabling -fexplicit-export due to use of export directive");
		scas_runtime.options.explicit_export = 1;
	}
	int i;
	for (i = 0; i < argc; ++i) {
		scas_log(L_INFO, "Exporting '%s' via %s:%d", argv[i],
				stack_peek(state->file_name_stack),
				*(int*)stack_peek(state->line_number_stack));
		char *exported = malloc(strlen(argv[i]) + 1);
		strcpy(exported, argv[i]);
		list_add(state->object->exports, exported);
	}
	return 1;
}

int handle_import(struct assembler_state *state, char **argv, int argc) {
	if (!scas_runtime.options.explicit_import) {
		WARN(WARNING_NO_EFFECT, state->column,
				".import", "explicit_import is not enabled");
	}
	else {
		for (int i = 0; i < argc; ++i) {
			scas_log(L_DEBUG, "Importing '%s'",argv[i]);
			char *imported = malloc(strlen(argv[i]) + 1);
			strcpy(imported, argv[i]);
			list_add(state->object->imports, imported);
		}
	}
	return 1;
}

int handle_if(struct assembler_state *state, char **argv, int argc) {
	if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
		/* Push up another falsy if if we're already in a falsy if */
		int *r = malloc(sizeof(int));
		*r = 0;
		stack_push(state->if_stack, r);
		return 1;
	}
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "if expects 1 argument");
		return 1;
	}
	tokenized_expression_t *expression = parse_expression(argv[0]);
	int error;
	char *symbol;
	uint64_t result;
	if (expression == NULL) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else {
		result = evaluate_expression(expression, state->equates, &error, &symbol);
		free_expression(expression);
	}
	if (error == EXPRESSION_BAD_SYNTAX) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
		return 1;
	} else {
		int *r = malloc(sizeof(int));
		*r = result;
		stack_push(state->if_stack, r);
		scas_log(L_DEBUG, "IF directive evaluated to %d", result);
	}
	return 1;
}

int handle_ifdef(struct assembler_state *state, char **argv, int argc) {
	if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
		/* Push up another falsy if if we're already in a falsy if */
		int *r = malloc(sizeof(int));
		*r = 0;
		stack_push(state->if_stack, r);
		return 1;
	}
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "ifdef expects 1 argument");
		return 1;
	}
	int *r = malloc(sizeof(int));
	*r = 0;
	int i;
	for (i = 0; !*r && i < state->equates->length; ++i) {
		symbol_t *sym = state->equates->items[i];
		if (strcasecmp(sym->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	for (i = 0; !*r && i < state->current_area->symbols->length; ++i) {
		symbol_t *sym = state->current_area->symbols->items[i];
		if (strcasecmp(sym->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	for (i = 0; !*r && i < state->macros->length; ++i) {
		macro_t *m = state->macros->items[i];
		if (strcasecmp(m->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	stack_push(state->if_stack, r);
	scas_log(L_DEBUG, "IFDEF directive evaluated to %d", *r);
	return 1;
}

int handle_ifndef(struct assembler_state *state, char **argv, int argc) {
	if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
		/* Push up another falsy if if we're already in a falsy if */
		int *r = malloc(sizeof(int));
		*r = 0;
		stack_push(state->if_stack, r);
		return 1;
	}
	if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
		return 1;
	}
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "ifndef expects 1 argument");
		return 1;
	}
	int *r = malloc(sizeof(int));
	*r = 0;
	int i;
	for (i = 0; !*r && i < state->equates->length; ++i) {
		symbol_t *sym = state->equates->items[i];
		if (strcasecmp(sym->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	for (i = 0; !*r && i < state->current_area->symbols->length; ++i) {
		symbol_t *sym = state->current_area->symbols->items[i];
		if (strcasecmp(sym->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	for (i = 0; !*r && i < state->macros->length; ++i) {
		macro_t *m = state->macros->items[i];
		if (strcasecmp(m->name, argv[0]) == 0) {
			*r = 1;
		}
	}
	*r = !*r;
	stack_push(state->if_stack, r);
	scas_log(L_DEBUG, "IFNDEF directive evaluated to %d", *r);
	return 1;
}

int handle_incbin(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "incbin expects 1 argument");
		return 1;
	}
	/* TODO: Pass runtime settings down to assembler from main */
	int len = strlen(argv[0]);
	if ((argv[0][0] != '"' || argv[0][len - 1] != '"') &&
			(argv[0][0] != '<' || argv[0][len - 1] != '>')) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated \"\" or <>");
		return 1;
	}
	argv[0][len - 1] = '\0';
	len -= 2;
	len = unescape_string(argv[0] + 1);
	char *name = malloc(strlen(argv[0] + 1));
	strcpy(name, argv[0] + 1);
	FILE *file = fopen(name, "r");
	if (!file) {
		ERROR(ERROR_BAD_FILE, state->column, name);
		free(name);
		return 1;
	}
	scas_log(L_INFO, "Including binary file '%s' from '%s'", name,
			(char *)stack_peek(state->file_name_stack));
	uint8_t *buf = malloc(1024);
	uint64_t olen = 0;
	int l;
	while ((l = fread(buf, sizeof(uint8_t), 1024, file))) {
		append_to_area(state->current_area, buf, l);
		state->PC += l;
		olen += l;
	}
	if (!state->expanding_macro) {
		add_source_map((source_map_t *)stack_peek(state->source_map_stack),
			*(int*)stack_peek(state->line_number_stack), state->line,
			state->current_area->data_length, olen);
	}
	free(buf);
	fclose(file);
	return 1;
}

int handle_include(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "include expects 1 argument");
		return 1;
	}
	int len = strlen(argv[0]);
	if ((argv[0][0] != '"' || argv[0][len - 1] != '"') &&
			(argv[0][0] != '<' || argv[0][len - 1] != '>')) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated \"\" or <>");
		return 1;
	}
	argv[0][len - 1] = '\0';
	len -= 2;
	len = unescape_string(argv[0] + 1);
	char *name = malloc(strlen(argv[0] + 1) + 1);
	strcpy(name, argv[0] + 1);
	FILE *file = NULL;
	int i;
	for (i = -1; i < state->settings->include_path->length; ++i) {
		if (i == -1) {
			file = fopen(name, "r");
		} else {
			char *ip = state->settings->include_path->items[i];
			char *temp = malloc(strlen(name) + strlen(ip) + 2);
			*temp = 0;
			strcat(temp, ip);
			strcat(temp, "/");
			strcat(temp, name);
			scas_log(L_DEBUG, "Trying %s", temp);
			file = fopen(temp, "r");
			free(temp);
		}
		if (file) {
			break;
		}
	}
	if (!file) {
		ERROR(ERROR_BAD_FILE, state->column, name);
		return 1;
	}
	scas_log(L_INFO, "Including file '%s' from '%s'", name, (char *)stack_peek(state->file_name_stack));
	stack_push(state->file_stack, file);
	stack_push(state->file_name_stack, name);
	int *ln = malloc(sizeof(int)); *ln = 0;
	stack_push(state->line_number_stack, ln);
	stack_push(state->source_map_stack, create_source_map(state->current_area, name));
	return 1;
}

int handle_list(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "list expects 0 arguments");
		return 1;
	}
	state->nolist = 0;
	scas_log(L_DEBUG, "Resumed listing");
	return 1;
}

int handle_map(struct assembler_state *state, char **argv, int argc) {
	if (argc != 3) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "map expects three arguments");
	}
	// .map filename, lineno, code
	free(((source_map_t *)stack_peek(state->source_map_stack))->file_name);
	((source_map_t*)stack_peek(state->source_map_stack))->file_name = 
			malloc(strlen(argv[0]) + 1);
	strcpy(((source_map_t*)stack_peek(state->source_map_stack))->file_name,
			argv[0]);
	add_source_map((source_map_t *)stack_peek(state->source_map_stack),
		atoi(argv[1]), argv[2], state->PC, 1); // TODO: figure out actual length
	return 1;
}

int handle_macro(struct assembler_state *state, char **argv, int argc) {
	if (argc != 1) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "macro expects 1 argument");
		return 1;
	}
	char *location = strchr(argv[0], '(');

	if (location == argv[0]) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "macro without a name");
		return 1;
	}

	if (location && strchr(location + 1, '(') != NULL) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "macro contains excess parentheses");
		return 1;
	}

	macro_t *macro = malloc(sizeof(macro_t));
	macro->parameters = create_list();
	macro->macro_lines = create_list();

	if (location) {
		macro->name = malloc(location - argv[0] + 1);
		strncpy(macro->name, argv[0], location - argv[0]);
		macro->name[location - argv[0]] = 0;
		location++;
		while(*location) {
			char *end = strchr(location, ',');

			if (!end) {
				end = strchr(location, ')');
			}

			if (!end) {
				ERROR(ERROR_INVALID_DIRECTIVE, state->column, "unterminated parameter list");
				return 1;
				// TODO: Free everything
			}
			else if (end == location && *end == ')') {
				// No parameters
				break;
			}
			char *parameter = malloc(end - location + 1);
			strncpy(parameter, location, end - location);
			parameter[end - location] = 0;
			int _;
			parameter = strip_whitespace(parameter, &_);
			list_add(macro->parameters, parameter);
			if (*end == ')') {
				break;
			}
			location = end + 1;
		}
	} else {
		macro->name = malloc(strlen(argv[0]) + 1);
		strcpy(macro->name, argv[0]);
	}
	state->current_macro = macro;
	return 1;
}

int handle_nolist(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "nolist expects 0 arguments");
		return 1;
	}
	state->nolist = 1;
	scas_log(L_DEBUG, "Paused listing");
	return 1;
}

int handle_odd(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	if (argc != 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "odd expects 0 arguments");
		return 1;
	}
	if (state->PC % 2 != 1) {
		uint8_t pad = 0;
		MAP_SOURCE(1);
		append_to_area(state->current_area, &pad, sizeof(uint8_t));
		++state->PC;
		scas_log(L_DEBUG, "Added byte to pad for .odd directive");
	}
	return 1;
}

int handle_org(struct assembler_state *state, char **argv, int argc) {
	if (argc == 0) {
		ERROR(ERROR_INVALID_DIRECTIVE, state->column, "org expects 1 argument");
		return 1;
	}
	int error;
	uint64_t result;
	char *symbol;
	tokenized_expression_t *expression = parse_expression(argv[0]);
	if (!expression) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
		return 1;
	} else {
		result = evaluate_expression(expression, state->equates, &error, &symbol);
		free_expression(expression);
	}
	if (error == EXPRESSION_BAD_SYMBOL) {
		ERROR(ERROR_UNKNOWN_SYMBOL, state->column, symbol);
	} else if (error == EXPRESSION_BAD_SYNTAX) {
		ERROR_NO_ARG(ERROR_INVALID_SYNTAX, state->column);
	} else {
		state->PC = result;
		scas_log(L_DEBUG, "Set origin to 0x%08X from org directive", state->PC);
	}
	return 1;
}

int handle_optsdcc(struct assembler_state *state, char **argv, int argc) {
	(void)argv;
	(void)argc;
	// For now this is a hack to turn off automatic source maps
	state->auto_source_maps = false;
	return 1;
}

/* Keep these alphabetized */
struct directive directives[] = {
	{ "area", handle_area, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "ascii", handle_ascii, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "asciip", handle_asciip, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "asciiz", handle_asciiz, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "blkb", handle_block, DELIM_COMMAS },
	{ "block", handle_block, DELIM_COMMAS },
	{ "bndry", handle_bndry, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "byte", handle_db, DELIM_COMMAS },
	{ "db", handle_db, DELIM_COMMAS },
	{ "define", handle_define, 0 },
	{ "dl", handle_dl, DELIM_COMMAS },
	{ "ds", handle_block, DELIM_COMMAS },
	{ "dw", handle_dw, DELIM_COMMAS },
	{ "echo", handle_echo, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "elif", handle_elseif, 0 },
	{ "else", handle_else, 0 },
	{ "elseif", handle_elseif, 0 },
	{ "end", handle_end, 0 },
	{ "endif", handle_endif, 0 },
	{ "equ", handle_equ, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "equate", handle_equ, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "even", handle_even, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "export", handle_export, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "fill", handle_fill, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "function", handle_function, DELIM_COMMAS },
	{ "gblequ", handle_equ, DELIM_COMMAS | DELIM_WHITESPACE }, /* TODO: Allow users to export equates? */
	{ "global", handle_export, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "globl", handle_export, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "if", handle_if, 0 },
	{ "ifdef", handle_ifdef, 0 },
	{ "ifndef", handle_ifndef, 0 },
	{ "import", handle_import, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "incbin", handle_incbin, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "include", handle_include, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "lclequ", handle_equ, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "list", handle_list, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "local", handle_nop, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "macro", handle_macro, 0 },
	{ "map", handle_map, DELIM_COMMAS },
	{ "module", handle_nop, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "nolist", handle_nolist, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "odd", handle_odd, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "optsdcc", handle_optsdcc, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "org", handle_org, 0 },
	{ "printf", handle_printf, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "ref", handle_nop, DELIM_COMMAS | DELIM_WHITESPACE }, /* TODO */
	{ "rmb", handle_block, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "rs", handle_block, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "section", handle_area, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "strs", handle_ascii, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "strz", handle_asciiz, DELIM_COMMAS | DELIM_WHITESPACE },
	{ "undef", handle_undef, DELIM_WHITESPACE },
	{ "undefine", handle_undef, DELIM_WHITESPACE },
};

struct directive if_directives[] = { /* The only directives parsed during a falsey if stack */
	{ "elif", handle_elseif, 0 },
	{ "else", handle_else, 0 },
	{ "elseif", handle_elseif, 0 },
	{ "end", handle_end, 0 },
	{ "endif", handle_endif, 0 },
	{ "if", handle_if, 0 },
	{ "ifdef", handle_ifdef, 0 },
	{ "ifndef", handle_ifndef, 0 },
};

int directive_compare(const void *_a, const void *_b) {
	const struct directive *a = _a;
	const struct directive *b = _b;
	return strcasecmp(a->match, b->match);
}

static struct directive nop = { "!", handle_nop, 0 };
struct directive *find_directive(struct directive dirs[], int l, char *line) {
	if (line[1] == '!') {
		return &nop;
	}
	++line;
	int whitespace = 0;
	while (line[whitespace] && !isspace(line[whitespace++]));
	if (line[whitespace]) {
		--whitespace;
	}
	char b = line[whitespace];
	line[whitespace] = '\0';
	struct directive d = { .match=line };
	struct directive *res = bsearch(&d, dirs, l, sizeof(struct directive), directive_compare);
	line[whitespace] = b;
	return res;
}

char **split_directive(char *line, int *argc, int delimiter) {
	const char *delimiters;
	*argc = 0;
	while (isspace(*line) && *line) ++line;
	/*
	 * Directives can be delimited with whitespace OR with commas.
	 * If you use commas at all, commas will be used exclusively.
	 */
	int capacity = 10;
	char **parts = malloc(sizeof(char *) * capacity);
	if (!*line) return parts;
	if (code_strchr(line, ',') != NULL && delimiter & DELIM_COMMAS) {
		delimiters = delimiters_list[DELIM_COMMAS];
	} else {
		delimiters = delimiters_list[delimiter];
	}
	int in_string = 0, in_character = 0;
	int i, j, _;
	for (i = 0, j = 0; line[i]; ++i) {
		if (line[i] == '\\') {
			++i;
		} else if (line[i] == '"' && !in_character) {
			in_string = !in_string;
		} else if (line[i] == '\'' && !in_string) {
			in_character = !in_character;
		} else if (!in_character && !in_string) {
			if (strchr(delimiters, line[i]) != NULL) {
				char *item = malloc(i - j + 1);
				strncpy(item, line + j, i - j);
				item[i - j] = '\0';
				item = strip_whitespace(item, &_);
				if (item[0] == '\0') {
					free(item);
				} else {
					if (*argc == capacity) {
						capacity *= 2;
						parts = realloc(parts, sizeof(char *) * capacity);
					}
					parts[*argc] = item;
					j = i + 1;
					++*argc;
				}
			}
		}
	}
	char *item = malloc(i - j + 1);
	strncpy(item, line + j, i - j);
	item[i - j] = '\0';
	if (*argc == capacity) {
		capacity++;
		parts = realloc(parts, sizeof(char *) * capacity);
	}
	parts[*argc] = strip_whitespace(item, &_);
	++*argc;
	return parts;
}

void correct_equates(char **line) {
	if (**line == '.') {
		return; // Already valid form
	}
	if (code_strchr(*line, '=')) {
		/* TODO */
	} else {
		int space = code_strchr(*line, ' ') - *line;
		if (space < 0) space = INT_MAX;
		int tab = code_strchr(*line, '\t') - *line;
		if (tab < 0) tab = INT_MAX;
		if (tab < space) space = tab;
		if (space == INT_MAX) return;
		char *name = malloc(space + 1);
		strncpy(name, *line, space);
		name[space] = '\0';
		/* Skip to value */
		while ((*line)[space] && isspace((*line)[space])) space++;
		while ((*line)[space] && !isspace((*line)[space])) space++;
		while ((*line)[space] && isspace((*line)[space])) space++;
		const char *fmtstring = ".equ %s %s";
		char *new_line = malloc(
			(strlen(fmtstring) - 4) +
			strlen(*line + space) +
			strlen(name) +
			1);
		sprintf(new_line, fmtstring, name, *line + space);
		scas_log(L_DEBUG, "Rewrote '%s' to '%s'", *line, new_line);
		free(*line);
		free(name);
		*line = new_line;
	}
}

int try_handle_directive(struct assembler_state *state, char **line) {
	/* Handle alternate forms of .equ */
	if (!(**line == '.' || **line == '#')) {
		if (code_strstr(*line, ".equ") || code_strchr(*line, '=')) {
			correct_equates(line);
		}
	}
	if (**line == '.' || **line == '#') {
		struct directive *dir = directives;
		int len = sizeof(directives) / sizeof(struct directive);
		if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
			dir = if_directives;
			len = sizeof(if_directives) / sizeof(struct directive);
		}
		struct directive *d = find_directive(dir, len, *line);
		if (d == NULL) {
			if (state->if_stack->length != 0 && !*(int *)stack_peek(state->if_stack)) {
				// Ignore "invalid" directives inside of falsy if segments
				return 1;
			}
			ERROR(ERROR_INVALID_DIRECTIVE, state->column, *line);
			return 1;
		}
		scas_log(L_DEBUG, "Matched directive '%s' at %s:%d", d->match,
				(char *)stack_peek(state->file_name_stack), *(int *)stack_peek(state->line_number_stack));
		int argc;
		char **argv = split_directive(*line + strlen(d->match) + 1, &argc, d->delimiter);
		indent_log();
		int ret = d->function(state, argv, argc);
		deindent_log();
		for (int i = 0; i < argc; i += 1) {
			free(argv[i]);
		}
		free(argv);
		return ret;
	}
	return 0;
}
