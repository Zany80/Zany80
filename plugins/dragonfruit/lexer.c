#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#include "stretchy_buffer.h"
#include "SIMPLE/scas/stringop.h"
#include "SIMPLE/scas/list.h"

#include "lexer.h"
#include "internals.h"

const char *get_token_str(lexer_token_t token) {
	static const char *strings[TOKEN_COUNT] = {
		"keyc", "error", "string", "number", "tag", "finished", "map"
	};
	return strings[token];
}

lexer_t *lexer_new(const char *src) {
	lexer_t *l = malloc(sizeof(lexer_t));
	// Capacity starts at 1MB. The ring buffer will automatically grow as needed
	l->buffer = ring_buffer_new(1024 * 1024);
	l->current_index = 0;
	l->tokens = create_list();
	l->types = create_list();
	lexer_insert(l, src);
	return l;
}

char lexer_extract_char(lexer_t *lexer) {
	char c = ring_buffer_read(lexer->buffer);
	if (c == '\n' && current_file != NULL) {
		char buf[16];
		sprintf(buf, "%d", current_line++);
		// Check if there's already a map present - if so, just
		// modify it instead of inserting new tokens.
		int prev_index = lexer->current_index - 3;
		if (prev_index >= 0 && prev_index <= lexer->tokens->length
				&& *(lexer_token_t*)lexer->types->items[prev_index] == map) {
			if (strcmp((char*)lexer->tokens->items[prev_index + 1], current_file)) {
				free(lexer->tokens->items[prev_index + 1]);
				lexer->tokens->items[prev_index + 1] = strdup(current_file);
			}
			free(lexer->tokens->items[prev_index + 2]);
			lexer->tokens->items[prev_index + 2] = strdup(buf);
		}
		else {
			lexer_token_t *type = malloc(sizeof(lexer_token_t));
			*type = map;
			list_insert(lexer->types, lexer->current_index, type);
			list_insert(lexer->tokens, lexer->current_index++, strdup("map"));
			type = malloc(sizeof(lexer_token_t));
			*type = string;
			list_insert(lexer->types, lexer->current_index, type);
			list_insert(lexer->tokens, lexer->current_index++, strdup(current_file));
			type = malloc(sizeof(lexer_token_t));
			*type = number;
			list_insert(lexer->types, lexer->current_index, type);
			list_insert(lexer->tokens, lexer->current_index++, strdup(buf));
		}
	}
	return c;
}

const char *const kc = "!@#()[]";

static size_t peek_index = 0;

bool peek_getchar(lexer_t *lexer, char *c) {
	if (peek_index >= ring_buffer_available(lexer->buffer)) {
		return false;
	}
	*c = ring_buffer_peek(lexer->buffer, peek_index++);
	return true;
}

bool extract_getchar(lexer_t *lexer, char *c) {
	if (ring_buffer_available(lexer->buffer) > 0) {
		*c = lexer_extract_char(lexer);
		return true;
	}
	return false;
}

static bool isnumber(const char *t) {
	char *endptr;
	strtol(t, &endptr, 0);
	return *t != 0 && *endptr == 0;
}

static lexer_token_t type_dump;

static void lexer_generic_extract(lexer_t *lexer, char **token, lexer_token_t *type, getchar_t getchar) {
	peek_index = 0;
	char c[5];
	c[1] = 0;
	do {
		if (!getchar(lexer, c)) {
			*type = finished;
			return;
		}
	} while (isspace(c[0]));
	if (strstr(kc, c) != NULL) {
		*token = strdup(c);
		*type = keyc;
		return;
	}
	char *t = NULL;
	if (c[0] == '"') {
		do {
			if (!getchar(lexer, c)) {
				*token = strdup("malformed string");
				*type = error;
				return;
			}
			if (c[0] == '\\') {
				if (!getchar(lexer, c)) {
					*token = strdup("malformed string: escape");
					*type = error;
					return;
				}
				switch (c[0]) {
					case '\\':
						sb_push(t, '\\');
						break;
					case 'n':
						sb_push(t, '\n');
						break;
					case 't':	
						sb_push(t, '\t');
						break;
					case '[':
						sb_push(t, (char)0x1b);
						break;
					default:
						sb_push(t, c[0]);
						c[0] = 0;
						break;
				}
			}
			else if (c[0] != '"') {
				sb_push(t, c[0]);
			}
		} while (c[0] != '"');
		sb_push(t, 0);
		*token = strdup(t);
		sb_free(t);
		*type = string;
		return;
	}
	if (c[0] == '\'') {
		sb_free(t);
		if (getchar(lexer, c)) {
			if (c[0] == '\\') {
				if (getchar(lexer, c)) {
					switch (c[0]) {
						case 'n':
							c[0] = '\n';
							break;
						case 't':
							c[0] = '\t';
							break;
						case 'b':
							c[0] = '\b';
							break;
						case '[':
							c[0] = 0x1b;
							break;
					}
				}
				else {
					*type = error;
					*token = strdup("unexpected EOF!");
					return;
				}
			}
			sprintf(c, "%d", c[0]);
			char dump;
			if (getchar(lexer, &dump)) {
				if (dump == '\'') {
					*token = strdup(c);
					*type = number;
				}
				else {
					*type = error;
					*token = strdup("malformed char");
				}
				return;
			}
		}
		*type = error;
		*token = strdup("unexpected EOF!");
		return;
	}
	do {
		sb_push(t, c[0]);
	} while (getchar(lexer, c) && !isspace(c[0]) && strstr(kc, c) == NULL);
	sb_push(t, 0);
	*token = strdup(t);
	sb_free(t);
	if (strstr(kc, c) != NULL) {
		if (getchar == extract_getchar) {
			ring_buffer_prepend(lexer->buffer, c[0]);
		}
		else {
			peek_index--;
		}
	}
	*type = isnumber(*token) ? number : tag;
}

void lexer_insert(lexer_t *lexer, const char *str) {
	ring_buffer_prepend_buf(lexer->buffer, str, strlen(str));
	int o_index =lexer->current_index;
	while (true) {
		char *token;
		lexer_token_t *type = malloc(sizeof(lexer_token_t));
		lexer_generic_extract(lexer, &token, type, extract_getchar);
		if (*type == finished) {
			free(type);
			break;
		}
		list_insert(lexer->tokens, lexer->current_index, token);
		list_insert(lexer->types, lexer->current_index++, type);
		//~ printf("New token inserted at index %d, offset %d\n", lexer->current_index + offset, offset);
	}
	lexer->current_index = o_index;
}

void lexer_rewind(lexer_t *lexer, int count) {
	lexer->current_index -= count;
}

void lexer_peek(lexer_t *lexer, char **token, lexer_token_t *type) {
	if (type == NULL) {
		type = &type_dump;
	}
	if (lexer->current_index < lexer->tokens->length) {
		*token = (char*)(lexer->tokens->items[lexer->current_index]);
		*type = *((lexer_token_t*)lexer->types->items[lexer->current_index]);
	}
	else {
		*type = finished;
	}
}

void lexer_extract(lexer_t *lexer, char **token, lexer_token_t *type) {
	if (type == NULL) {
		type = &type_dump;
	}
	if (lexer->current_index < lexer->tokens->length) {
		*token = (char*)(lexer->tokens->items[lexer->current_index]);
		*type = *((lexer_token_t*)lexer->types->items[lexer->current_index]);
		lexer->current_index++;
	}
	else {
		*type = finished;
	}
}

void lexer_destroy(lexer_t *lexer) {
	//~ size_t ring_buffer_size = ring_buffer_available(lexer->buffer);
	//~ char *buf = malloc(ring_buffer_size + 128);
	//~ if (!buf) {
		//~ fputs("OOM\n", stderr);
		//~ exit(1);
	//~ }
	//~ int size = sprintf(buf, "Remaining in lexer buffer at destruction: 0x%lx/%lu. Buffer contents: \"", ring_buffer_available(lexer->buffer), ring_buffer_available(lexer->buffer));
	//~ ring_buffer_read_buf(lexer->buffer, buf + size, ring_buffer_size);
	//~ buf[size + ring_buffer_size] = 0;
	//~ strcat(buf + size + ring_buffer_size, "\"");
	//~ print(buf);
	free_flat_list(lexer->tokens);
	free_flat_list(lexer->types);
	ring_buffer_free(lexer->buffer);
	free(lexer);
}
