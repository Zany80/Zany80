#pragma once

#define INSTRUCTION_COUNT 40
#include "lexer.h"

typedef struct auto_t auto_t;

typedef struct {
	const char *name;
	void (*handler)();
} instruction_t;

typedef struct {
	const char *name;
	instruction_t instructions[INSTRUCTION_COUNT];
	void (*function_call)(const char *name);
	void (*function_header)(const char *name);
	void (*function_footer)();
	void (*append_branch)(const char *name);
	void (*append_string)(const char *name, const char *targets);
	void (*append_const)(const char *name, const char *value);
	void (*stack)(char *var);
	// returns true if there remain unallocated registers
	bool (*has_autos)();
	// get an auto register
	auto_t*(*get_auto)();
	void (*stack_auto)(auto_t*);
	void (*assign_auto)(auto_t*);
	// reset backend internal state
	void (*reset)();
	// if the value on top of the stack is equal to value, branch to target
	void (*conditional_branch_equal)(int value, const char *target);
	// emit a branch to target
	void (*unconditional_branch)(const char *target);
	// table_header and table_value must build a table in a temporary buffer
	// table_finish flushes the table out using append_data and frees the memory
	// table_header should perform any needed allocations
	void (*table_header)(const char *name);
	void (*table_value)(const char *value);
	void (*table_finish)();
} compiler_backend_t;

extern compiler_backend_t limn;
