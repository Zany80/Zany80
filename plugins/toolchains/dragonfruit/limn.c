#include "backends.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stretchy_buffer.h>

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

static void limn_bswap() {
	append_compiled("\tpopv r5, r0\n\tbswap r0, r0\n\tpushv r5, r0\n");
}

static void limn_equality() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tcmp r0, r1\n\tandi r0, rf, 0x01\n\tpushv r5, r0\n");
}

static void limn_inequality() {
	append_compiled("\tpopv r5, r0\n\tpopv r5, r1\n\tcmp r0, r1\n\tnot rf\n\tandi r0, rf, 0x01\n\tpushv r5, r0\n");	
}

static void limn_assign() {
	append_compiled("\tpopv r5, r1\n\tpopv r5, r0\n\tsrr.l r1, r0\n");
}

static void limn_return() {
	append_compiled("\tret\n");
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
			.name = "!",
			.handler = limn_assign
		},
		{
			.name = "return",
			.handler = limn_return
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
