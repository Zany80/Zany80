#include "expression.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "readline.h"
#include "stack.h"
#include "stringop.h"
#include "operators.h"
#include "objects.h"

static operator_t operators[] = {
	{ "+", OP_UNARY_PLUS, 1, 3, 1, operator_unary_plus },
	{ "-", OP_UNARY_MINUS, 1, 3, 1, operator_unary_minus },
	{ "~", OP_NEGATE, 1, 3, 1, operator_negate },
	{ "!", OP_LOGICAL_NOT, 1, 3, 1, operator_logical_not },
	{ "*", OP_MULTIPLY, 0, 5, 0, operator_multiply },
	{ "/", OP_DIVIDE, 0, 5, 0, operator_divide },
	{ "%", OP_MODULO, 0, 5, 0, operator_modulo },
	{ "+", OP_PLUS, 0, 6, 0, operator_add },
	{ "-", OP_MINUS, 0, 6, 0, operator_subtract },
	{ "<<", OP_LEFT_SHIFT, 0, 7, 0, operator_left_shift },
	{ ">>", OP_RIGHT_SHIFT, 0, 7, 0, operator_right_shift },
	{ "<=", OP_LESS_THAN_OR_EQUAL_TO, 0, 8, 0, operator_less_than_or_equal_to },
	{ ">=", OP_GREATER_THAN_OR_EQUAL_TO, 0, 8, 0, operator_greater_than_or_equal_to },
	{ "<", OP_LESS_THAN, 0, 8, 0, operator_less_than },
	{ ">", OP_GREATER_THAN, 0, 8, 0, operator_greater_than },
	{ "==", OP_EQUAL_TO, 0, 9, 0, operator_equal_to },
	{ "!=", OP_NOT_EQUAL_TO, 0, 9, 0, operator_not_equal_to },
	{ "&", OP_BITWISE_AND, 0, 10, 0, operator_bitwise_and },
	{ "^", OP_BITWISE_XOR, 0, 11, 0, operator_bitwise_xor },
	{ "|", OP_BITWISE_OR, 0, 12, 0, operator_bitwise_or },
	{ "&&", OP_LOGICAL_AND, 0, 13, 0, operator_logical_and },
	{ "||", OP_LOGICAL_OR, 0, 14, 0, operator_logical_or },
};

void print_tokenized_expression(FILE *f, tokenized_expression_t *expression) {
	int i;
	for (i = 0; i < expression->tokens->length; ++i) {
		expression_token_t *token = expression->tokens->items[i];
		switch (token->type) {
			case SYMBOL:
				fprintf(f, "'%s'", token->symbol);
				break;
			case NUMBER:
				fprintf(f, "0x%04X", (unsigned int)token->number);
				break;
			case OPERATOR:
				fprintf(f, "[(%snary) %s]", operators[token->operator].is_unary ? "u" :
						"bi", operators[token->operator].operator);
				break;
		}
		if (i != expression->tokens->length - 1) {
			fputc(' ', f);
		}
	}
}

void fwrite_tokens(FILE *f, tokenized_expression_t *expression) {
	uint32_t len = expression->tokens->length;
	fwrite(&len, sizeof(uint32_t), 1, f);
	int i;
	for (i = 0; i < expression->tokens->length; ++i) {
		expression_token_t *token = expression->tokens->items[i];
		fputc(token->type, f);
		switch (token->type) {
			case SYMBOL:
				fprintf(f, "%s", token->symbol);
				fputc(0, f);
				break;
			case NUMBER:
				fwrite(&token->number, sizeof(uint64_t), 1, f);
				break;
			case OPERATOR:
				fputc(token->operator, f);
				break;
		}
	}
}

tokenized_expression_t *fread_tokenized_expression(FILE *f) {
	uint32_t len;
	fread(&len, sizeof(uint32_t), 1, f);
	tokenized_expression_t *expression = malloc(sizeof(tokenized_expression_t));
	expression->tokens = create_list();
	int i;
	for (i = 0; i < len; ++i) {
		expression_token_t *token = malloc(sizeof(expression_token_t));
		token->type = fgetc(f);
		switch (token->type) {
			case SYMBOL:
				token->symbol = read_line(f);
				break;
			case NUMBER:
				fread(&token->number, sizeof(uint64_t), 1, f);
				break;
			case OPERATOR:
				token->operator = fgetc(f);
				break;
		}
		list_add(expression->tokens, token);
	}
	return expression;
}

uint64_t evaluate_expression(tokenized_expression_t *expression, list_t
		*symbols, int *error, char **symbol) {
	stack_type *stack = create_stack();
	list_t *to_free = create_list();
	expression_token_t *token;
	expression_token_t *resolved;
	operator_t op;
	uint64_t res = 0;
	*error = 0;

	int i;
	for (i = 0; i < expression->tokens->length; ++i) {
		expression_token_t *token = expression->tokens->items[i];
		switch (token->type) {
			case SYMBOL:
				resolved = malloc(sizeof(expression_token_t));
				resolved->type = NUMBER;
				resolved->number = 0;
				int found = 0;
				int j;
				for (j = 0; j < symbols->length; ++j) {
					symbol_t *sym = symbols->items[j];
					if (strcasecmp(sym->name, token->symbol) == 0) {
						resolved->number = sym->value;
						found = 1;
						break;
					}
				}
				if (!found) {
					*symbol = token->symbol;
					*error = EXPRESSION_BAD_SYMBOL;
				}
				list_add(to_free, resolved);
				stack_push(stack, resolved);
				break;
			case NUMBER:
				stack_push(stack, token);
				break;
			case OPERATOR:
				op = operators[token->operator];
				resolved = malloc(sizeof(expression_token_t));
				resolved->type = NUMBER;
				resolved->number = op.function(stack, error);
				list_add(to_free, resolved);
				stack_push(stack, resolved);
				break;
		}
	}

	if (stack->length == 0) {
		*error = 1;
	} else {
		token = stack_pop(stack);
		if (token->type != NUMBER) {
			*error = 1;
		} else {
			res = token->number;
		}
	}
	stack_free(stack);
	free_flat_list(to_free);
	return res;
}

expression_token_t *parse_digit(const char **string) {
	if ((*string)[0] == '0') {
		int base = 10;
		switch ((*string)[1]) {
		case 'b':
			base = 2;
			break;
		case 'x':
			base = 16;
			break;
		case 'o':
			base = 8;
			break;
		case 0:
			// It's probably a single digit number
			--*string;
			break;
		default:
			return 0;
			break;
		}
		*string += 2;

		expression_token_t *expr = malloc(sizeof(expression_token_t));
		expr->type = NUMBER;
		char *end;
		expr->number = strtol(*string, &end, base);
		*string = end;
		return expr;
	}

	/* Character literals */
	if ((*string)[0] == '\'') {
		if (strlen(*string) >= 3) {
			char *end = strchr(*string + 1, '\''); // Note: this doesn't work with '\'' etc
			if (end) {
				char *temp = malloc(end - *string + 1);
				strncpy(temp, *string, end - *string);
				temp[end - *string] = '\0';
				unescape_string(temp);
				expression_token_t *expr = malloc(sizeof(expression_token_t));
				expr->type = NUMBER;
				expr->number = (uint64_t)temp[1];
				*string = end + 1;
				return expr;
			}
		}
	}

	/* ASxxxx-style local labels */
	const char *a = *string;
	while (*a) {
		if (*a == '$') {
			return NULL;
		}
		++a;
	}

	/* Plain decimal number */
	expression_token_t *expr = malloc(sizeof(expression_token_t));
	expr->type = NUMBER;
	char *end;
	expr->number = strtol(*string, &end, 10);
	*string = end;
	return expr;
}

expression_token_t *parse_operator(const char **string, int is_unary) {
	int i;
	for (i = 0; i < sizeof(operators) / sizeof(operator_t); i++) {
		operator_t op = operators[i];
		if (op.is_unary == is_unary && strncmp(op.operator, *string, strlen(op.operator)) == 0) {
			expression_token_t *exp = malloc(sizeof(expression_token_t));
			exp->type = OPERATOR;
			exp->operator = i;
			*string += strlen(op.operator);
			return exp;
		}
	}

	return 0;
}

expression_token_t *parse_symbol(const char **string) {
	const char *end = *string;
	while (*end && (isalnum(*end) || *end == '_' || *end == '.' || *end == '@' || *end == '$')) {
		end++;
	}
	char *symbol = malloc(end - *string + 1);
	strncpy(symbol, *string, end - *string);
	symbol[end - *string] = '\0';
	*string = end;

	expression_token_t *expr = malloc(sizeof(expression_token_t));
	expr->type = SYMBOL;
	expr->symbol = symbol;
	return expr;
}

enum {
	STATE_VALUE,
	STATE_OPERATOR,
};

// Based on shunting-yard
tokenized_expression_t *parse_expression(const char *str) {
	char *operator_cache = malloc(sizeof(operators) / sizeof(operator_t) + 1);
	int i;
	for (i = 0; i < sizeof(operators) / sizeof(operator_t); i++) {
		operator_t op = operators[i];
		operator_cache[i] = op.operator[0];
	}
	operator_cache[i] = 0;

	int tokenizer_state = STATE_OPERATOR;
	tokenized_expression_t *list = malloc(sizeof(tokenized_expression_t));
	list->tokens = create_list();
	list->symbols = create_list();
	stack_type *stack = create_stack();

	const char *current = str;
	while (1) {
		while (*current == '#' || isspace(*(current))) {
			/* Note: # is used for immediate data in ASxxxx, it's somewhat superfluous */
			current++;
		}
		expression_token_t *expr;
		if (*current == 0) {
			break;
		} else if (isdigit(*current) || *current == '\'') {
			if (tokenizer_state == STATE_VALUE) {
				return 0;
			}

			expr = parse_digit(&current);
			if (expr == NULL) {
				expr = parse_symbol(&current);
				list_add(list->symbols, expr->symbol);
			}
			tokenizer_state = STATE_VALUE;
		} else if (*current == '(') {
			if (tokenizer_state == STATE_VALUE) {
				return 0;
			}
			expr = malloc(sizeof(expression_token_t));
			expr->type = OPEN_PARENTHESIS;
			stack_push(stack, expr);
			current++;
			tokenizer_state = STATE_OPERATOR;
			continue;
		} else if (*current == ')') {
			if (stack->length == 0 || tokenizer_state == STATE_OPERATOR) {
				return 0;
			}
			expr = stack->items[stack->length - 1];
			while (expr && expr->type != OPEN_PARENTHESIS) {
				stack_pop(stack);
				list_add(list->tokens, expr);

				if (stack->length <= 0) {
					expr = 0;
					return 0;
				}

				expr = stack_peek(stack);
			}

			stack_pop(stack);
			free(expr);
			current++;
			tokenizer_state = STATE_VALUE;
			continue;
		} else if(strchr(operator_cache, *current)) {
			expr = parse_operator(&current, tokenizer_state == STATE_OPERATOR);
			if (expr == 0) {
				return 0;
			}
			tokenizer_state = STATE_OPERATOR;
		} else {
			if (tokenizer_state == STATE_VALUE) {
				return 0;
			}
			expr = parse_symbol(&current);
			list_add(list->symbols, expr->symbol);
			tokenizer_state = STATE_VALUE;
		}

		if (!expr) {
			return 0;
		}

		if (expr->type == OPERATOR) {
			operator_t *operator = &operators[expr->operator];
			expression_token_t *expr2 = stack->length ? stack_peek(stack) : 0;
			operator_t *operator2 = expr2 ? &operators[expr2->operator] : 0;
			while (expr2 && expr2->type == OPERATOR && ((!operator->right_assocative && operator2->precedence == operator->precedence) || operator->precedence > operator2->precedence)) {
				stack_pop(stack);
				list_add(list->tokens, expr2);
				if (!stack->length) {
					break;
				}
				expr2 = stack_peek(stack);
				operator2 = &operators[expr2->operator];
			}
			stack_push(stack, expr);
		} else {
			list_add(list->tokens, expr);
		}
	}

	while (stack->length > 0) {
		expression_token_t *item = stack_pop(stack);
		list_add(list->tokens, item);
	}

	stack_free(stack);
	free(operator_cache);
	return list;
}
