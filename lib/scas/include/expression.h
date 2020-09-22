#ifndef EXPRESSION_H
#define EXPRESSION_H
#include "list.h"
#include "stack.h"
#include <stdint.h>
#include <stdio.h>

enum {
	SYMBOL = 0x00,
	NUMBER = 0x01,
	OPERATOR = 0x02,
	OPEN_PARENTHESIS
};

enum {
	EXPRESSION_BAD_SYMBOL = 1,
	EXPRESSION_BAD_SYNTAX = 2
};

enum {
	OPERATOR_UNARY = 1,
	OPERATOR_BINARY = 2,
};

typedef struct {
	int type;
	char *symbol;
	uint64_t number;
	int operator;
} expression_token_t;

typedef struct {
	list_t *tokens;
	list_t *symbols;
} tokenized_expression_t;

typedef struct {
	char *operator;
	int expression_type;
	int is_unary;
	int precedence;
	int right_assocative;
	uint64_t (*function)(stack_type *, int *);
} operator_t;

// NOTE: when passing operators, pass them sorted by length, largest first.
// Otherwise the parser may parse e.g. '>>' as '>' '>'.
tokenized_expression_t *parse_expression(const char *str);
void free_expression(tokenized_expression_t *expression);
void initialize_expressions();
void print_tokenized_expression(FILE *f, tokenized_expression_t *expression);
uint64_t evaluate_expression(tokenized_expression_t *expression, list_t
        *symbols, int *error, char **symbol);
void fwrite_tokens(FILE *f, tokenized_expression_t *expression);
tokenized_expression_t *fread_tokenized_expression(FILE *f);
void free_expression_token(expression_token_t *token);
int get_relative_label_offset(tokenized_expression_t *expression, unsigned int *start);

#endif
