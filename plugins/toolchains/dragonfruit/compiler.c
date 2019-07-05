#include <3rd-party/scas/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stretchy_buffer.h>
#include "lexer.h"
#include "backends.h"

char **buffer;
size_t buffer_size, buffer_capacity;

void print(const char *string) {
	puts(string);
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

char * append_str(char *target, const char *str) {
	if (target != NULL) {
		sb_pop(target, 1);
	}
	for (size_t i = 0; i < strlen(str); i++) {
		sb_push(target, str[i]);
	}
	sb_push(target, 0);
	return target;
}

void append_compiled(const char *str) {
	compiled = append_str(compiled, str);
}

void append_data(const char *str) {
	compiled_data = append_str(compiled_data, str);
}

static char *current_file;
static int current_line;
static compiler_backend_t *compiler_backend;

void compiler_error(const char *format_str, ...) __attribute__((format(printf, 1, 2)));

void compiler_error(const char *format_str, ...) {
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

void map_line() {
	char *buf = malloc(7 + strlen(current_file) + 32 + 1);
	sprintf(buf, ".map %s, %d\n", current_file, ++current_line);
	append_compiled(buf);
	free(buf);
}

static lexer_t *lexer;

void extract(char **token, lexer_token_t *type) {
	lexer_extract(lexer, token, type);
}

lexer_token_t peek_type() {
	char *token = NULL;
	lexer_token_t type;
	lexer_peek(lexer, &token, &type);
	if (token)
		free(token);
	return type;
}

typedef struct {
	const char *name;
	void (*handler)();
} directive_t;

void directive_include() {
	char *path = NULL;
	lexer_token_t type;
	lexer_extract(lexer, &path, &type);
	if (type != string) {
		compiler_error("Expected string as include path, received %s", get_token_str(type));
	}
	else {
		FILE *f = fopen(path, "r");
		if (f) {
			fseek(f, 0, SEEK_END);
			long size = ftell(f);
			rewind(f);
			char *buf = malloc(size + 1);
			fread(buf, 1, size, f);
			fclose(f);
			buf[size] = 0;
			char *b2 = malloc(21 + strlen(path) + 1 + strlen(buf) + strlen(current_file) + 10 + 1);
			sprintf(b2, "#path \"%s\" %d\n%s\n#path \"%s\" %d\n", path, 0, buf, current_file, current_line);
			lexer_insert(lexer, b2);
			free(buf);
			free(b2);
		}
		else {
			compiler_error("Error opening included file %s", path);
		}
	}
	if (path != NULL) {
		free(path);
	}
}

void directive_path() {
	char *token = NULL;
	lexer_token_t type;
	lexer_extract(lexer, &token, &type);
	if (type != string) {
		compiler_error("Unexpected %s received as parameter 0 to path, expected string", get_token_str(type));
	}
	else {
		char *_file = token;
		token = NULL;
		lexer_extract(lexer, &token, &type);
		if (type != number) {
			compiler_error("Unexpected %s received as parameter 1 to path, expected number", get_token_str(type));
		}
		else {
			if (current_file) {
				free(current_file);
			}
			current_file = _file;
			current_line = strtol(token, NULL, 0);
		}
	}
	if (token != NULL) {
		free(token);
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
	// transfers ownership of name to vars. Avoids an unneeded strdup/free pair
	sb_push(vars, name);
	free(initv);
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
	sb_push(auto_names, strdup(name));
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
	free_flat_sb(auto_names);
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
	if (token != NULL) {
		free(token);
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
	if (token != NULL) {
		free(token);
	}
	char false_branch[32];
	new_branch(false_branch);
	compile_block(")");
	compiler_backend->conditional_branch_equal(0, false_branch);
	compile_block("end");
	lexer_peek(lexer, &token, NULL);
	if (token != NULL && !strcmp(token, "else")) {
		free(token);
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
	if (token != NULL) {
		free(token);
	}
}

char **consts, **const_values;

void clean_vars() {
	for (int i = 0; i < sb_count(consts); i++) {
		compiler_warning("Const %d of %d: %s = %s", i + 1, sb_count(consts), consts[i], const_values[i]);
	}
	free_flat_sb(consts);
	free_flat_sb(const_values);
	free_flat_sb(vars);
}

// value should be a pointer to a dynamically allocated string. It may be 
// reallocated by compile_const.
// exception: if type is known to be a number, value can be statically allocated
void compile_const(char *name, char **value, lexer_token_t type) {
	if (type != number) {
		if (type != string) {
			compiler_error("Unexpected %s at const value", get_token_str(type));
		}
		else {
			char label[32];
			new_const(label);
			compiler_backend->append_string(label, *value);
			free(*value);
			*value = strdup(label);
		}
	}
	compiler_backend->append_const(name, *value);
	sb_push(consts, strdup(name));
	sb_push(const_values, strdup(*value));
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
	compile_const(name, &initv, type);
	free(name);
	free(initv);
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
			free(token);
			break;
		}
		if (type != number) {
			if (type == tag && have_const(token)) {
				char *ntoken = strdup(const_values[find_const(token)]);
				free(token);
				token = ntoken;
			}
			else {
				compiler_error("expected number or const in struct, received %s: %s", get_token_str(type), token);
				free(token);
				free(name);
				return;
			}
		}
		char *member;
		extract(&member, &type);
		if (type != tag) {
			compiler_error("expected tag in struct, received %s", get_token_str(type));
			free(name);
			free(token);
			return;
		}
		char *buf = malloc(strlen(name) + 1 + strlen(member) + 1);
		sprintf(buf, "%s_%s", name, member);
		// on stack with cast to avoid unneeded heap allocation and fre
		char off_buf[32];
		char *o = off_buf;
		sprintf(off_buf, "%d", offset);
		offset += strtol(token, NULL, 0);
		compile_const(buf, &o, number);
		free(buf);
		free(token);
	}
	char *buf = malloc(strlen(name) + 8 + 1);
	sprintf(buf, "%s_SIZEOF", name);
	// on stack with cast to avoid unneeded heap allocation and fre
	char off_buf[32];
	char *o = off_buf;
	sprintf(off_buf, "%d", offset);
	compile_const(buf, &o, number);
	free(buf);
	free(name);
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
	sb_push(vars, name);
	while (true) {
		char *token = NULL;
		extract(&token, &type);
		if (token == NULL) {
			compiler_error("Expected endtable, found EOF");
			break;
		}
		if (!strcmp(token, "endtable")) {
			free(token);
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
					free(fname);
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
		free(token);
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
	if (name != NULL) {
		free(name);
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
		lexer_extract(lexer, &token, NULL);
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
		free(token);
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
	else if (have_const(token)) {
		compiler_backend->stack(token);
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
	while (peek_type() != finished) {
		lexer_extract(lexer, &token, &type);
		if (end && !strcmp(token, end)) {
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
		free(token);
	}
}

void startup() {
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
}

void finish_up() {
	sb_free(compiled_data);
	sb_free(compiled);
	clean_autos();
	// also takes care of consts
	clean_vars();
	free(current_file);
	(*buffer)[--buffer_size] = 0;
}

char *read_file(const char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		compiler_error("opening source file %s", path);
		return NULL;
	}
	if (current_file != NULL) {
		free(current_file);
	}
	current_file = strdup(path);
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
	char *buf = read_file((const char *)(sources->items[0]));
	if (!buf) {
		return 2;
	}
	lexer = lexer_new(buf);
	free(buf);
	for (int i = 1; i < sources->length; i++) {
		const char *file_name = (const char *)(sources->items[i]);
		char *buf = malloc(12 + strlen(file_name) + 1);
		sprintf(buf, "#include \"%s\"\n", file_name);
		lexer_insert(lexer, buf);
		free(buf);
	}
	map_line();
	compile_block(NULL);
	lexer_destroy(lexer);
	append_compiled("\n\n");
	if (compiled_data != NULL) {
		append_compiled(compiled_data);
	}
	print("Final compiled output: ");
	print(compiled);
	//~ puts(compiled);
	finish_up();
	return -1;
}
