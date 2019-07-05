#pragma once

#include <stdbool.h>

typedef struct lexer_t lexer_t;

typedef enum {
	keyc, error, string, number, tag, finished,
	TOKEN_COUNT
} lexer_token_t;

const char *get_token_str(lexer_token_t token);

typedef bool(*getchar_t)(lexer_t *lexer, char *c);

lexer_t *lexer_new(const char *src);
void lexer_insert(lexer_t *lexer, const char *str);
char lexer_extract_char(lexer_t *lexer);
void lexer_peek(lexer_t *lexer, char **token, lexer_token_t *type);
void lexer_extract(lexer_t *lexer, char **token, lexer_token_t *type);
void lexer_destroy(lexer_t *lexer);
