#include "lexer.h"
#include <stddef.h>
#include <stdlib.h>
#include <ring_buffer.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stretchy_buffer.h>
#include <stdbool.h>

void print(const char *string);
void map_line();

struct lexer_t {
	ring_buffer_t *buffer;
};

const char *get_token_str(lexer_token_t token) {
	static const char *strings[TOKEN_COUNT] = {
		"keyc", "error", "string", "number", "tag", "finished"
	};
	return strings[token];
}

lexer_t *lexer_new(const char *src) {
	lexer_t *l = malloc(sizeof(lexer_t));
	// Capacity starts at 1MB. The ring buffer will automatically grow as needed
	l->buffer = ring_buffer_new(1024 * 1024);
	lexer_insert(l, src);
	return l;
}

void lexer_insert(lexer_t *lexer, const char *str) {
	ring_buffer_prepend_buf(lexer->buffer, str, strlen(str));
}

char lexer_extract_char(lexer_t *lexer) {
	char c = ring_buffer_read(lexer->buffer);
	if (c == '\n') {
		map_line();
	}
	return c;
}

char *kc = "!@#()[]";

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
		*c = ring_buffer_read(lexer->buffer);
		if (*c == '\n') {
			map_line();
		}
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
	if (type == NULL) {
		type = &type_dump;
	}
	peek_index = 0;
	char c[2];
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

void lexer_peek(lexer_t *lexer, char **token, lexer_token_t *type) {
	lexer_generic_extract(lexer, token, type, peek_getchar);
}

void lexer_extract(lexer_t *lexer, char **token, lexer_token_t *type) {
	lexer_generic_extract(lexer, token, type, extract_getchar);
}

void lexer_destroy(lexer_t *lexer) {
	size_t ring_buffer_size = ring_buffer_available(lexer->buffer);
	char *buf = malloc(ring_buffer_size + 128);
	if (!buf) {
		fputs("OOM\n", stderr);
		exit(1);
	}
	int size = sprintf(buf, "Remaining in lexer buffer at destruction: 0x%lx/%lu. Buffer contents: \"", ring_buffer_available(lexer->buffer), ring_buffer_available(lexer->buffer));
	ring_buffer_read_buf(lexer->buffer, buf + size, ring_buffer_size);
	buf[size + ring_buffer_size] = 0;
	strcat(buf + size + ring_buffer_size, "\"");
	print(buf);
	ring_buffer_free(lexer->buffer);
	free(lexer);
}
