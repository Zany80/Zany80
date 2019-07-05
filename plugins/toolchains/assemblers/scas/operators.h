#ifndef OPERATORS_H
#define OPERATORS_H
#include <stdint.h>
#include "stack.h"

enum {
    OP_PLUS = 1,
    OP_MINUS,
    OP_UNARY_PLUS,
    OP_UNARY_MINUS,
    OP_NEGATE,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_LOGICAL_NOT,
    OP_MODULO,
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,
    OP_LESS_THAN,
    OP_GREATER_THAN,
    OP_LESS_THAN_OR_EQUAL_TO,
    OP_GREATER_THAN_OR_EQUAL_TO,
    OP_EQUAL_TO,
    OP_NOT_EQUAL_TO,
	OP_BITWISE_AND,
	OP_BITWISE_XOR,
	OP_BITWISE_OR,
	OP_LOGICAL_AND,
	OP_LOGICAL_OR,
};

uint64_t operator_add(stack_type *stack, int *error);
uint64_t operator_subtract(stack_type *stack, int *error);
uint64_t operator_multiply(stack_type *stack, int *error);
uint64_t operator_divide(stack_type *stack, int *error);
uint64_t operator_unary_plus(stack_type *stack, int *error);
uint64_t operator_unary_minus(stack_type *stack, int *error);
uint64_t operator_negate(stack_type *stack, int *error);
uint64_t operator_logical_not(stack_type *stack, int *error);
uint64_t operator_modulo(stack_type *stack, int *error);
uint64_t operator_left_shift(stack_type *stack, int *error);
uint64_t operator_right_shift(stack_type *stack, int *error);
uint64_t operator_less_than(stack_type *stack, int *error);
uint64_t operator_greater_than(stack_type *stack, int *error);
uint64_t operator_less_than_or_equal_to(stack_type *stack, int *error);
uint64_t operator_greater_than_or_equal_to(stack_type *stack, int *error);
uint64_t operator_equal_to(stack_type *stack, int *error);
uint64_t operator_not_equal_to(stack_type *stack, int *error);
uint64_t operator_bitwise_and(stack_type *stack, int *error);
uint64_t operator_bitwise_xor(stack_type *stack, int *error);
uint64_t operator_bitwise_or(stack_type *stack, int *error);
uint64_t operator_logical_and(stack_type *stack, int *error);
uint64_t operator_logical_or(stack_type *stack, int *error);

#endif
