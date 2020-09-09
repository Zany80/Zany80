#pragma once

#include <stdbool.h>
#include <SIMPLE/ring_buffer.h>
#include <SIMPLE/scas/list.h>
#include "lexer.h"

void print(const char *string);
void append_compiled(const char *str);
void append_data(const char *str);
char * append_str(char *target, const char *str);
lexer_token_t peek_type();
void extract(char **token, lexer_token_t *type);
void compile_block(const char *end);
void compiler_error(const char *format_str, ...) __attribute__((format(printf, 1, 2)));
void compiler_warning(const char *format_str, ...) __attribute__((format(printf, 1, 2)));
bool have_const(const char *name);
char *get_const(const char *name);
void add_var(char *name);

extern char *current_file;
extern int current_line;

struct lexer_t {
	ring_buffer_t *buffer;
	list_t *tokens;
	list_t *types;
	int current_index;
};

struct optimizer_t {
	lexer_t *lexer;
};
