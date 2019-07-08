#include <3rd-party/scas/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stretchy_buffer.h>
#include "lexer.h"
#include "backends.h"
#include "internals.h"

static char **buffer;
static size_t buffer_size, buffer_capacity;

// Since c
static char **local_strings;

static char *base_dir;

static char *read_file(char *path);

static int error_count;

void print(const char *string) {
	//~ puts(string);
	size_t size = strlen(string);
	if (buffer_size + size >= buffer_capacity) {
		char *new_buffer = realloc(*buffer, buffer_capacity *= 2);
		if (!new_buffer) {
			fputs("OUT OF MEMORY!", stderr);
			exit(1);
		}
		*buffer = new_buffer;
	}
	while (size-- > 0) {
		(*buffer)[buffer_size++] = *string;
		string++;
	}
	(*buffer)[buffer_size++] = '\n';
	(*buffer)[buffer_size] = 0;
}

static char *compiled;
// used for e.g. strings
static char *compiled_data;

static int current_str, current_branch, current_const;

static void new_str(char *dest) {
	sprintf(dest, "__dragonfruit_str_%d", current_str++);
}

static void new_branch(char *dest) {
	sprintf(dest, "__dragonfruit_branch_%d", current_branch++);
}

static void new_const(char *dest) {
	sprintf(dest, "__dragonfruit_const_%d", current_const++);
}

char *append_str(char *target, const char *str) {
	for (size_t i = 0; i < strlen(str); i++) {
		sb_push(target, str[i]);
	}
	return target;
}

static bool c_map_printed, d_map_printed;
static char *current_map = NULL;

void append_compiled(const char *str) {
	if (!c_map_printed && current_map != NULL) {
		c_map_printed = true;
		compiled = append_str(compiled, current_map);
	}
	compiled = append_str(compiled, str);
}

void append_data(const char *str) {
	if (!d_map_printed && current_map != NULL) {
		d_map_printed = true;
		compiled_data = append_str(compiled_data, current_map);
	}
	compiled_data = append_str(compiled_data, str);
}

char *current_file;
int current_line;
static compiler_backend_t *compiler_backend;

void compiler_error(const char *format_str, ...) {
	error_count++;
	char buf[1024];
	int index = 0;
	if (current_file != NULL) {
		index = snprintf(buf, 1023, "%s:%d: error: ", current_file, current_line);
	}
	va_list args;
	va_start(args, format_str);
	vsnprintf(buf + index, 1023 - index, format_str, args);
	va_end(args);
	print(buf);
}

void compiler_warning(const char *format_str, ...) __attribute__((format(printf, 1, 2)));

void compiler_warning(const char *format_str, ...) {
	char buf[1024];
	int index = 0;
	if (current_file != NULL) {
		index = snprintf(buf, 1023, "%s:%d: warning: ", current_file, current_line);
	}
	va_list args;
	va_start(args, format_str);
	vsnprintf(buf + index, 1023 - index, format_str, args);
	va_end(args);
	print(buf);
}

void compile_map() {
	c_map_printed = d_map_printed = false;
	if (current_map != NULL) {
		free(current_map);
	}
	char *file;
	char *line;
	lexer_token_t type;
	extract(&file, &type);
	assert(type == string);
	extract(&line, &type);
	assert(type == number);
	current_map = malloc(8 + strlen(file) + strlen(line) + 1);
	sprintf(current_map, ".map %s, %s\n", file, line);
}

static lexer_t *lexer;

void extract(char **token, lexer_token_t *type) {
	lexer_extract(lexer, token, type);
	while (type != NULL && *type == map) {
		compile_map();
		lexer_extract(lexer, token, type);
	}
}

void peek(char **token, lexer_token_t *type) {
	lexer_peek(lexer, token, type);
}

lexer_token_t peek_type() {
	char *token = NULL;
	lexer_token_t type;
	lexer_peek(lexer, &token, &type);
	return type;
}

typedef struct {
	const char *name;
	void (*handler)();
} directive_t;

void directive_include() {
	char *path = NULL;
	lexer_token_t type;
	extract(&path, &type);
	if (type != string) {
		compiler_error("Expected string as include path, received %s", get_token_str(type));
	}
	else {
		char *full_path = malloc(strlen(base_dir) + strlen(path) + 2);
		strcat(strcat(strcpy(full_path, base_dir), "/"), path);
		char *buf = read_file(full_path);
		sb_push(local_strings, full_path);
		if (buf) {
			char *b2 = malloc(21 + strlen(path) + 1 + strlen(buf) + strlen(current_file) + 10 + 1);
			sprintf(b2, "#path \"%s\" %d\n%s\n#path \"%s\" %d\n", path, 0, buf, current_file, current_line);
			lexer_insert(lexer, b2);
			free(buf);
			free(b2);
		}
		else {
			compiler_error("Error opening included file %s", full_path);
		}
	}
}

void directive_path() {
	char *token = NULL;
	lexer_token_t type;
	extract(&token, &type);
	if (type != string) {
		compiler_error("Unexpected %s received as parameter 0 to path, expected string", get_token_str(type));
	}
	else {
		char *_file = token;
		extract(&token, &type);
		if (type != number) {
			compiler_error("Unexpected %s received as parameter 1 to path, expected number", get_token_str(type));
		}
		else {
			current_file = _file;
			current_line = strtol(token, NULL, 0);
			c_map_printed = d_map_printed = false;
		}
	}
}

directive_t directives[2] = {
	{
		.name = "include",
		.handler = directive_include
	},
	{
		.name = "path",
		.handler = directive_path
	}
};

directive_t *find_directive(const char *name) {
	for (size_t i = 0; i < sizeof(directives) / sizeof(directives[0]); i++) {
		if (!strcmp(directives[i].name, name)) {
			return directives + i;
		}
	}
	return NULL;
}

char **vars;

void add_var(char *name) {
	sb_push(vars, name);
}

bool have_var(char *name) {
	for (int i = 0; i < sb_count(vars); i++) {
		if (!strcmp(name, vars[i])) {
			return true;
		}
	}
	return false;
}

void instruction_var() {
	char *name = NULL, *initv = NULL;
	lexer_token_t name_type, initv_type;
	extract(&name, &name_type);
	extract(&initv, &initv_type);
	if (name_type != tag) {
		compiler_error("Unexpected %s at var", get_token_str(name_type));
	}
	if (initv_type != number) {
		compiler_error("Unexpected %s at var", get_token_str(initv_type));
	}
	if (!name || !initv) {
		return;
	}
	append_data(name);
	append_data(":\n\t.dl ");
	append_data(initv);
	append_data("\n");
	add_var(name);
}

static char **auto_names;
static auto_t **autos;

void instruction_auto() {
	char *name;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_error("Unexpected %s at auto", get_token_str(type));
	}
	if (!compiler_backend->has_autos()) {
		compiler_error("auto: out of registers");
		return;
	}
	for (int i = 0; i < sb_count(auto_names); i++) {
		if (!strcmp(auto_names[i], name)) {
			compiler_warning("redeclaration of auto %s has no effect", name);
			return;
		}
	}
	sb_push(auto_names, name);
	sb_push(autos, compiler_backend->get_auto());
}

void free_flat_sb(void *_sb) {
	// allows usage of void* parameter for implicit casting in parameter
	void **sb = (void**)_sb;
	for (int i = 0; i < sb_count(sb); i++) {
		free(sb[i]);
	}
	sb_free(sb);
}

void clean_autos() {
	sb_free(auto_names);
	free_flat_sb(autos);
	auto_names = NULL;
	autos = NULL;
}

void instruction_procedure() {
	char *name;
	lexer_token_t token;
	extract(&name, &token);
	if (token != tag) {
		compiler_error("Unexpected %s in place of procedure name", get_token_str(token));
	}
	compiler_backend->function_header(name);
	compile_block("end");
	compiler_backend->function_footer();
	clean_autos();
}

void instruction_asm() {
	char *token;
	lexer_token_t type;
	extract(&token, &type);
	if (type == tag && !strcmp(token, "preamble")) {
		compiler_warning("Preamble not yet supported");
	}
	else {
		append_compiled("; asm block\n");
		append_compiled(token);
		append_compiled("\n");
	}
}

static char **whiles;

void instruction_while() {
	char *token = NULL;
	extract(&token, NULL);
	if (token == NULL || token[0] != '(') {
		compiler_error("malformed while");
	}
	char cond_branch[32];
	char exit_branch[32];
	new_branch(cond_branch);
	new_branch(exit_branch);
	compiler_backend->append_branch(cond_branch);
	compile_block(")");
	compiler_backend->conditional_branch_equal(0, exit_branch);
	sb_push(whiles, exit_branch);
	compile_block("end");
	sb_pop(whiles, 1);
	compiler_backend->unconditional_branch(cond_branch);
	compiler_backend->append_branch(exit_branch);
}

void instruction_break() {
	if (sb_count(whiles) > 0) {
		compiler_backend->unconditional_branch(sb_last(whiles));
	}
}

void instruction_if() {
	char *token = NULL;
	extract(&token, NULL);
	if (token == NULL || token[0] != '(') {
		compiler_error("malformed if");
	}
	char false_branch[32];
	new_branch(false_branch);
	compile_block(")");
	compiler_backend->conditional_branch_equal(0, false_branch);
	compile_block("end");
	lexer_peek(lexer, &token, NULL);
	if (token != NULL && !strcmp(token, "else")) {
		extract(&token, NULL);
		char exit_branch[32];
		new_branch(exit_branch);
		compiler_backend->unconditional_branch(exit_branch);
		compiler_backend->append_branch(false_branch);
		compile_block("end");
		compiler_backend->append_branch(exit_branch);
	}
	else {
		compiler_backend->append_branch(false_branch);
	}
}

char **consts, **const_values;

void clean_vars() {
	sb_free(consts);
	sb_free(const_values);
	sb_free(vars);
}

void compile_const(char *name, char *value, lexer_token_t type) {
	char *rvalue = value;
	char label[32];
	if (type != number) {
		if (type != string) {
			compiler_error("Unexpected %s at const value", get_token_str(type));
		}
		else {
			new_const(label);
			compiler_backend->append_string(label, value);
			rvalue = label;
		}
	}
	compiler_backend->append_const(name, rvalue);
	sb_push(consts, name);
	sb_push(const_values, value);
}

void instruction_const() {
	char *name;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_warning("Unexpected %s at const", get_token_str(type));
	}
	char *initv;
	extract(&initv, &type);
	compile_const(name, initv, type);
}

bool have_const(const char *name) {
	for (int i = 0; i < sb_count(consts); i++) {
		if (!strcmp(consts[i], name)) {
			return true;
		}
	}
	return false;
}

int find_const(const char *name) {
	for (int i = 0; i < sb_count(consts); i++) {
		if (!strcmp(consts[i], name)) {
			return i;
		}
	}
	return -1;
}

char *get_const(const char *name) {
	int index = find_const(name);
	if (index == -1) {
		return NULL;
	}
	return const_values[index];
}

void instruction_struct() {
	char *name;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_warning("unexpected %s at tag", get_token_str(type));
	}
	unsigned offset = 0;
	while (true) {
		char *token = NULL;
		extract(&token, &type);
		if (token == NULL) {
			compiler_warning("expected endstruct at EOF");
			break;
		}
		if (!strcmp(token, "endstruct")) {
			break;
		}
		if (type != number) {
			if (type == tag && have_const(token)) {
				token = get_const(token);
			}
			else {
				compiler_error("expected number or const in struct, received %s: %s", get_token_str(type), token);
				return;
			}
		}
		char *member;
		extract(&member, &type);
		if (type != tag) {
			compiler_error("expected tag in struct, received %s", get_token_str(type));
			return;
		}
		char *buf = malloc(strlen(name) + 1 + strlen(member) + 1);
		sprintf(buf, "%s_%s", name, member);
		// on stack with cast to avoid unneeded heap allocation and fre
		char *o = malloc(32);
		sprintf(o, "%d", offset);
		offset += strtol(token, NULL, 0);
		compile_const(buf, o, number);
		// transfers ownership of buf and o to local_strings
		sb_push(local_strings, buf);
		sb_push(local_strings, o);
	}
	char *buf = malloc(strlen(name) + 8 + 1);
	sprintf(buf, "%s_SIZEOF", name);
	// on stack with cast to avoid unneeded heap allocation and fre
	char *o = malloc(32);
	sprintf(o, "%d", offset);
	compile_const(buf, o, number);
	// transfers ownership of buf and o to local_strings
	sb_push(local_strings, buf);
	sb_push(local_strings, o);
}

void instruction_table() {
	char *name;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_error("Expected table name tag, received %s", get_token_str(type));
	}
	// transfers ownership of name to the vars array. It will be freed by clean_vars
	compiler_backend->table_header(name);
	add_var(name);
	while (true) {
		char *token = NULL;
		extract(&token, &type);
		if (token == NULL) {
			compiler_error("Expected endtable, found EOF");
			break;
		}
		if (!strcmp(token, "endtable")) {
			break;
		}
		if (type == number) {
			compiler_backend->table_value(token);
		}
		else if (type == tag) {
			if (!strcmp(token, "pointerof")) {
				char *fname;
				lexer_token_t ftype;
				extract(&fname, &ftype);
				if (ftype != tag) {
					compiler_error("Expected function tag at pointerof in table, received %s", get_token_str(ftype));
				}
				if (fname != NULL) {
					compiler_backend->table_value(fname);
				}
			}
			else {
				if (have_const(token)) {
					compiler_backend->table_value(const_values[find_const(token)]);
				}
				else {
					compiler_error("Expected const, number, string, or function pointer, received value of type %s", get_token_str(type));
				}
			}
		}
		else if (type == string) {
			char str_name[32];
			new_str(str_name);
			compiler_backend->append_string(str_name, token);
			compiler_backend->table_value(str_name);
		}
		else {
			compiler_error("Expected const, number, string, or function pointer, received value of type %s", get_token_str(type));
		}
	}
	compiler_backend->table_finish();
}

void instruction_pointerof() {
	char *name = NULL;
	lexer_token_t type;
	extract(&name, &type);
	if (type != tag) {
		compiler_error("Expected function tag at pointerof, received %s", get_token_str(type));
	}
	else {
		compiler_backend->stack(name);
	}
}

#define SHARED_INSTRUCTION_COUNT 11
instruction_t shared_instructions[SHARED_INSTRUCTION_COUNT] = {
	{
		.name = "auto",
		.handler = instruction_auto
	},
	{
		.name = "var",
		.handler = instruction_var
	},
	{
		.name = "procedure",
		.handler = instruction_procedure
	},
	{
		.name = "asm",
		.handler = instruction_asm
	},
	{
		.name = "while",
		.handler = instruction_while
	},
	{
		.name = "break",
		.handler = instruction_break
	},
	{
		.name = "if",
		.handler = instruction_if
	},
	{
		.name = "const",
		.handler = instruction_const
	},
	{
		.name = "struct",
		.handler = instruction_struct
	},
	{
		.name = "table",
		.handler = instruction_table
	},
	{
		.name = "pointerof",
		.handler = instruction_pointerof
	},
};

instruction_t *find_instruction(const char *name) {
	for (size_t i = 0; i < SHARED_INSTRUCTION_COUNT; i++) {
		if (!strcmp(shared_instructions[i].name, name)) {
			return shared_instructions + i;
		}
	}
	for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
		if (!strcmp(compiler_backend->instructions[i].name, name)) {
			return compiler_backend->instructions + i;
		}
	}
	return NULL;
}

void compile_keyc(char *token) {
	if (token[0] == '#') {
		if (peek_type() == finished) {
			compiler_error("Unexpected EOF searching for directive!");
			return;
		}
		token = NULL;
		extract(&token, NULL);
		if (token == NULL) {
			compiler_error("error processing directive");
			return;
		}
		directive_t *directive = find_directive(token);
		if (directive != NULL) {
			directive->handler();
		}
		else {
			compiler_error("No directive found");
		}
	}
	else {
		instruction_t *instruction = find_instruction(token);
		if (instruction != NULL) {
			instruction->handler();
		}
		else {
			compiler_warning("Unknown keyc received for processing");
		}
	}
}

bool have_auto(char *token) {
	for (int i = 0; i < sb_count(auto_names); i++) {
		if (!strcmp(auto_names[i], token)) {
			return true;
		}
	}
	return false;
}

int auto_index(char *token) {
	for (int i = 0; i < sb_count(auto_names); i++) {
		if (!strcmp(auto_names[i], token)) {
			return i;
		}
	}
	return -1;
}

void compile_auto(char *auto_token) {
	int i = auto_index(auto_token);
	auto_t *a = autos[i];
	lexer_token_t type;
	char *token;
	extract(&token, &type);
	if (type != keyc) {
		compiler_error("unexpected %s after auto reference", get_token_str(type));
	}
	else {
		if (token[0] == '!') {
			compiler_backend->assign_auto(a);
		}
		else if (token[0] == '@') {
			compiler_backend->stack_auto(a);
		}
		else {
			compiler_error("Unexpected %c operator on auto reference", token[0]);
		}
	}
}

void compile_word(char *token) {
	instruction_t *instruction = find_instruction(token);
	if (instruction != NULL) {
		instruction->handler();
	}
	else if (have_var(token)) {
		compiler_backend->stack(token);
	}
	else if (have_const(token)) {
		char *c = get_const(token);
		compiler_backend->stack(c);
	}
	else if (have_auto(token)) {
		compile_auto(token);
	}
	else {
		compiler_backend->function_call(token);
	}
}

void compile_string(char *str) {
	char buf[32];
	new_str(buf);
	compiler_backend->stack(buf);
	compiler_backend->append_string(buf, str);
}

void compile_number(char *token) {
	compiler_backend->stack(token);
}

void compile_block(const char *end) {
	char *token;
	lexer_token_t type;
	while (true) {
		extract(&token, &type);
		if (type == finished || (end && !strcmp(token, end))) {
			break;
		}
		switch (type) {
			case keyc:
				compile_keyc(token);
				break;
			case tag:
				compile_word(token);
				break;
			case string:
				compile_string(token);
				break;
			case number:
				compile_number(token);
				break;
			default:
				compiler_warning("Unhandled token type: %s", get_token_str(type));
				break;
		}
	}
}

void startup() {
	error_count = 0;
	compiled = NULL;
	compiled_data = NULL;
	current_str = current_branch = current_const = 0;
	auto_names = NULL;
	autos = NULL;
	compiler_backend = &limn;
	compiler_backend->reset();
	buffer_size = 0;
	*buffer = malloc(buffer_capacity = 1024);
	print("== dragonfruit compiler ==");
	current_line = 0;
	whiles = NULL;
	consts = NULL;
	const_values = NULL;
	vars = NULL;
	local_strings = NULL;
}

void finish_up() {
	if (current_map != NULL) {
		free(current_map);
	}
	free(base_dir);
	sb_free(compiled_data);
	sb_free(compiled);
	clean_autos();
	// also takes care of consts
	clean_vars();
	(*buffer)[--buffer_size] = 0;
	lexer_destroy(lexer);
	free_flat_sb(local_strings);
}

char *read_file(char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		compiler_error("Error opening source file %s", path);
		return NULL;
	}
	current_file = path;
	current_line = 0;
	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	rewind(f);
	char *buf = malloc(length + 1);
	fread(buf, length, 1, f);
	fclose(f);
	buf[length] = 0;
	return buf;
}

int compile(list_t *sources, const char *target, char **_buffer) {
	buffer = _buffer;
	current_file = NULL;
	startup();
	if (target == NULL || strlen(target) == 0 || sources->length == 0) {
		print("Must pass in at least one source and a target file!");
		return 1;
	}
	{
		char *path = strdup((char *)(sources->items[0]));
		char *last_slash = rindex(path, '/');
		if (last_slash) {
			*last_slash = 0;
			base_dir = path;
		}
		else {
			base_dir = strdup(".");
			free(path);
		}
	}
	char *buf = read_file((char *)(sources->items[0]));
	if (!buf) {
		return 2;
	}
	lexer = lexer_new(buf);
	free(buf);
	for (int i = 1; i < sources->length; i++) {
		const char *file_name = (const char *)(sources->items[i]);
		//~ if (!strcmp(file_name, "-O")) {
		
		//~ }
		//~ else {
			char *buf = malloc(12 + strlen(file_name) + 1);
			sprintf(buf, "#include \"%s\"\n", file_name);
			lexer_insert(lexer, buf);
			free(buf);
		//~ }
	}
	compile_block(NULL);
	append_compiled("\n\n");
	FILE *destination = fopen(target, "w");
	if (destination) {
		fwrite(compiled, 1, sb_count(compiled), destination);
		if (compiled_data != NULL) {
			fwrite(compiled_data, 1, sb_count(compiled_data), destination);
		}
		fflush(destination);
		fclose(destination);
	}
	else {
		print("Error opening destination file for saving!");
	}
	finish_up();
	return error_count;
}
