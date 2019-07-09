#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stretchy_buffer.h>

#include "backends.h"
#include "internals.h"

static int cur_auto = 6;
#define MAX_AUTO 25

static void function_header(const char *name) {
	append_compiled(name);
	append_compiled(":\n");
	cur_auto = 6;
}

static void function_footer() {
	append_compiled("\tret\n");
}

static void limn_return() {
	append_compiled("\tret\n");
}

static void limn_bswap() {
	append_compiled("\tpopv r5, r0\n\tbswap r0, r0\n\tpushv r5, r0\n");
}

static void limn_equality() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tcmp r0, r1\n\tandi r0, rf, 0x01\n\tpushv r5, r0\n");
}

static void limn_inequality() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tcmp r0, r1\n\tnot rf, rf\n\tandi r0, rf, 0x01\n\tpushv r5, r0\n");	
}

static void limn_greater_than() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmp r0, r1\n");
	append_compiled("\trshi r0, rf, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void limn_less_than() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmp r0, r1\n");
	append_compiled("\tnot r1, rf\n");
	append_compiled("\trshi r0, r1, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tnot rf, rf\n");
	append_compiled("\tand r0, r0, rf\n");
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void limn_greater_or_equal() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmp r0, r1\n");
	append_compiled("\tmov r0, rf\n");
	append_compiled("\trshi rf, rf, 1\n"); // bitwise magic
	append_compiled("\tior r0, r0, rf\n");
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void limn_less_or_equal() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmp r0, r1\n");
	append_compiled("\tnot rf, rf\n");
	append_compiled("\trshi r0, rf, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void s_limn_greater_than() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmps r0, r1\n");
	append_compiled("\trshi r0, rf, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void s_limn_less_than() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmps r0, r1\n");
	append_compiled("\tnot r1, rf\n");
	append_compiled("\trshi r0, r1, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tnot rf, rf\n");
	append_compiled("\tand r0, r0, rf\n");
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void s_limn_greater_or_equal() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmps r0, r1\n");
	append_compiled("\tmov r0, rf\n");
	append_compiled("\trshi rf, rf, 1\n"); // bitwise magic
	append_compiled("\tior r0, r0, rf\n");
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void s_limn_less_or_equal() {
	append_compiled("\tpopv r5, r1\n");
	append_compiled("\tpopv r5, r0\n");
	append_compiled("\tcmps r0, r1\n");
	append_compiled("\tnot rf, rf\n");
	append_compiled("\trshi r0, rf, 0x1\n"); // isolate gt bit in flag register
	append_compiled("\tandi r0, r0, 1\n");
	append_compiled("\tpushv r5, r0\n");
}

static void bit_limn_not() {
	append_compiled("\tpopv r5, r0\n\tnot r0, r0\n\tpushv r5, r0\n");
}

static void bool_limn_not() {
	append_compiled("\tpopv r5, r0\n\tnot r0, r0\n\tandi r0, r0, 1\n\tpushv r5, r0\n");
}

static void bit_limn_or() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tior r0, r0, r1\n\tpushv r5, r0\n");
}

static void bool_limn_or() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tior r0, r0, r1\n\tandi r0, r0, 1\n\tpushv r5, r0\n");
}

static void bit_limn_and() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tand r0, r0, r1\n\tpushv r5, r0\n");
}

static void bool_limn_and() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tand r0, r0, r1\n\tandi r0, r0, 1\n\tpushv r5, r0\n");
}

static void limn_right_shift() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\trsh r0, r0, r1\n\tpushv r5, r0\n");
}

static void limn_left_shift() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tlsh r0, r1, r0\n\tpushv r5, r0\n");
}

static void limn_dup() {
	append_compiled("\tpopv r5, r0\n\tpushv r5, r0\n\tpushv r5, r0\n");
}

static void limn_swap() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tpushv r5, r0\n\tpushv r5, r1\n");
}

static void limn_drop() {
	append_compiled("\tpopv r5, r0\n");
}

static void limn_add() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tadd r0, r0, r1\n\tpushv r5, r0\n");
}

static void limn_subtract() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tsub r0, r1, r0\n\tpushv r5, r0\n");
}

static void limn_multiply() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tmul r0, r0, r1\n\tpushv r5, r0\n");
}

static void limn_divide() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tdiv r0, r1, r0\n\tpushv r5, r0\n");
}

static void limn_modulus() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tmod r0, r1, r0\n\tpushv r5, r0\n");
}

static void limn_array_index() {
	compile_block("]");
	char *obj;
	lexer_token_t type;
	extract(&obj, &type);
	if (type != tag) {
		compiler_error("Expected object tag at array index, received %s", get_token_str(type));
	}
	else {
		append_compiled("\tpopv r5, r0\n\tmuli r0, r0, 4\n\taddi r0, r0, ");
		append_compiled(obj);
		append_compiled("\n\tpushv r5, r0\n");
	}
}

static void limn_comment() {
	while (true) {
		char *token = NULL;
		lexer_token_t type;
		extract(&token, &type);
		if (token == NULL) {
			compiler_error("Expected ) to close comment");
		}
		else {
			bool done = false;
			if (!strcmp(token, ")")) {
				done = true;
			}
			if (done) {
				break;
			}
		}
	}
}

static void limn_getbyte() {
	append_compiled("\tpopv r5, r0\n\tlrr.b r0, r0\n\tpushv r5, r0\n");
}

static void limn_getint() {
	append_compiled("\tpopv r5, r0\n\tlrr.i r0, r0\n\tpushv r5, r0\n");
}

static void limn_getlong() {
	append_compiled("\tpopv r5, r0\n\tlrr.l r0, r0\n\tpushv r5, r0\n");	
}

static void limn_setbyte() {
	append_compiled("\tpopv r5, r1\n\tpopv r5, r0\n\tsrr.b r1, r0\n");
}

static void limn_setint() {
	append_compiled("\tpopv r5, r1\n\tpopv r5, r0\n\tsrr.i r1, r0\n");
}

static void limn_setlong() {
	append_compiled("\tpopv r5, r1\n\tpopv r5, r0\n\tsrr.l r1, r0\n");
}

static void limn_bitget() {
	append_compiled("\tpopv r5, r1\n\tpopv r5, r0\n\trsh r0, r0, r1\n\tandi r0, r0, 1\n\tpushv r5, r0\n");
}

static void limn_bitset() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tbset r1, r1, r0\n\tpushv r5, r1\n");
}

static void limn_bitclear() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tbclr r1, r1, r0\n\tpushv r5, r1\n");
}

static void limn_buffer() {
	char *name = NULL, *size = NULL;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_error("Expected buffer name tag, received %s", get_token_str(type));
	}
	extract(&size, &type);
	if (type != number) {
		if (type == tag && have_const(size)) {
			char *__size = get_const(size);
			size = __size;
		}
		else {
			compiler_error("Expected buffer size (number or const), received %s", get_token_str(type));
			return;
		}
	}
	add_var(name);
	append_data(name);
	append_data(":\n\t.bytes ");
	append_data(size);
	append_data(" 0x00\n");
}

static void function_call(const char *name) {
	for (int i = 6; i < cur_auto; i++) {
		char buf[16];
		sprintf(buf, "\tpush r%d\n", i);
		append_compiled(buf);
	}
	append_compiled("\tcall ");
	append_compiled(name);
	append_compiled("\n");
	for (int i = cur_auto; i > 6; i--) {
		char buf[16];
		sprintf(buf, "\tpop r%d\n", i - 1);
		append_compiled(buf);
	}
}

static void stack(char *var) {
	append_compiled("\tpushvi r5, ");
	append_compiled(var);
	append_compiled("\n");
}

static bool has_autos() {
	return cur_auto <= MAX_AUTO;
}

struct auto_t {
	int reg;
};

static auto_t *get_auto() {
	if (!has_autos()) {
		return NULL;
	}
	auto_t *a = malloc(sizeof(auto_t));
	a->reg = cur_auto++;
	return a;
}

static void limn_assign_auto(auto_t *a) {
	char buf[16];
	sprintf(buf, "\tpopv r5, r%d\n", a->reg);
	append_compiled(buf);
}

static void limn_stack_auto(auto_t *a) {
	char buf[16];
	sprintf(buf, "\tpushv r5, r%d\n", a->reg);
	append_compiled(buf);
}

static void append_branch(const char *name) {
	append_compiled(name);
	append_compiled(":\n");
}

static void append_string(const char *name, const char *str) {
	append_data(name);
	append_data(":\n\t.ds ");
	char buf[2];
	buf[1] = 0;
	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') {
			append_data("\n\t.db 0x0A\n\t.ds ");
		}
		else {
			buf[0] = str[i];
			append_data(buf);
		}
	}
	append_data("\n\t.db 0x00\n");
}

static void append_const(const char *name, const char *value) {
	append_data(name);
	append_data(" === ");
	append_data(value);
	append_data("\n");
}

static void conditional_branch_equal(int value, const char *target) {
	append_compiled("\tpopv r5, r0\n\tcmpi r0, ");
	char buf[20];
	sprintf(buf, "%d", value);
	append_compiled(buf);
	append_compiled("\n\tbe ");
	append_compiled(target);
	append_compiled("\n");
}

static void unconditional_branch(const char *target) {
	append_compiled("\tb ");
	append_compiled(target);
	append_compiled("\n");
}

static char *table_buf;
static void table_header(const char *name) {
	table_buf = append_str(table_buf, name);
	table_buf = append_str(table_buf, ":\n");
}

static void table_value(const char *value) {
	table_buf = append_str(table_buf, "\t.dl ");
	table_buf = append_str(table_buf, value);
	table_buf = append_str(table_buf, "\n");
}

static void table_finish() {
	sb_push(table_buf, 0);
	append_data(table_buf);
	sb_free(table_buf);
	table_buf = NULL;
}

static void reset() {
	cur_auto = 6;
	table_buf = NULL;
	// suppress warning, then free memory
	sb_push(table_buf, 0);
	sb_free(table_buf);
	table_buf = NULL;
}
compiler_backend_t limn = {
	.name = "limn",
	.instructions = {
		{
			.name = "return",
			.handler = limn_return
		},
		{
			.name = "bswap",
			.handler = limn_bswap
		},
		{
			.name = "==",
			.handler = limn_equality
		},
		{
			.name = "~=",
			.handler = limn_inequality
		},
		{
			.name = ">",
			.handler = limn_greater_than
		},
		{
			.name = "<",
			.handler = limn_less_than
		},
		{
			.name = ">=",
			.handler = limn_greater_or_equal
		},
		{
			.name = "<=",
			.handler = limn_less_or_equal
		},
		{
			.name = "s>",
			.handler = s_limn_greater_than
		},
		{
			.name = "s<",
			.handler = s_limn_less_than
		},
		{
			.name = "s>=",
			.handler = s_limn_greater_or_equal
		},
		{
			.name = "s<=",
			.handler = s_limn_less_or_equal
		},
		{
			.name = "~",
			.handler = bit_limn_not
		},
		{
			.name = "~~",
			.handler = bool_limn_not
		},
		{
			.name = "|",
			.handler = bit_limn_or
		},
		{
			.name = "||",
			.handler = bool_limn_or
		},
		{
			.name = "&",
			.handler = bit_limn_and
		},
		{
			.name = "&&",
			.handler = bool_limn_and
		},
		{
			.name = ">>",
			.handler = limn_right_shift
		},
		{
			.name = "<<",
			.handler = limn_left_shift
		},
		{
			.name = "dup",
			.handler = limn_dup
		},
		{
			.name = "swap",
			.handler = limn_swap
		},
		{
			.name = "drop",
			.handler = limn_drop
		},
		{
			.name = "+",
			.handler = limn_add
		},
		{
			.name = "-",
			.handler = limn_subtract
		},
		{
			.name = "*",
			.handler = limn_multiply
		},
		{
			.name = "/",
			.handler = limn_divide
		},
		{
			.name = "%",
			.handler = limn_modulus
		},
		{
			.name = "[",
			.handler = limn_array_index
		},
		{
			.name = "(",
			.handler = limn_comment
		},
		{
			.name = "gb",
			.handler = limn_getbyte
		},
		{
			.name = "gi",
			.handler = limn_getint
		},
		{
			.name = "@",
			.handler = limn_getlong
		},
		{
			.name = "sb",
			.handler = limn_setbyte
		},
		{
			.name = "si",
			.handler = limn_setint
		},
		{
			.name = "!",
			.handler = limn_setlong
		},
		{
			.name = "bitget",
			.handler = limn_bitget
		},
		{
			.name = "bitset",
			.handler = limn_bitset
		},
		{
			.name = "bitclear",
			.handler = limn_bitclear
		},
		{
			.name = "buffer",
			.handler = limn_buffer
		},
	},
	.function_call = function_call,
	.function_header = function_header,
	.function_footer = function_footer,
	.stack = stack,
	.reset = reset,
	.has_autos = has_autos,
	.get_auto = get_auto,
	.stack_auto = limn_stack_auto,
	.assign_auto = limn_assign_auto,
	.append_branch = append_branch,
	.append_string = append_string,
	.append_const = append_const,
	.conditional_branch_equal = conditional_branch_equal,
	.unconditional_branch = unconditional_branch,
	.table_header = table_header,
	.table_value = table_value,
	.table_finish = table_finish,
};
