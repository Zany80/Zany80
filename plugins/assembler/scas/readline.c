#include "readline.h"
#include <stdlib.h>
#include <stdio.h>

char *read_line(FILE *file) {
	int i = 0, length = 0, size = 128;
	char *string = malloc(size);
	if (!string) {
		return NULL;
	}
	while (1) {
		int c = getc(file);
		if (c == EOF || c == '\n' || c == '\0') {
			break;
		}
		if (c == '\r') {
			continue;
		}
		if (i == size) {
			char *_string = realloc(string, length *= 2);
			if (_string == NULL) {
				free(string);
				return NULL;
			}
			string = _string;
		}
		string[i++] = (char)c;
		length++;
	}
	if (i + 1 != size) {
		char *_string = realloc(string, length + 1);
		if (_string == NULL) {
			free(string);
			return NULL;
		}
		string = _string;
	}
	string[i] = '\0';
	return string;
}

char *read_line_s(const char *input, int *offset) {
	int i = 0, length = 0, size = 128;
	char *string = malloc(size);
	if (!string) {
		return NULL;
	}
	while (1) {
		int c = input[*offset];
		*offset += 1;
		if (c == EOF || c == '\n' || c == '\0') {
			break;
		}
		if (c == '\r') {
			continue;
		}
		if (i == size) {
			char *_string = realloc(string, length *= 2);
			if (_string == NULL) {
				free(string);
				return NULL;
			}
			string = _string;
		}
		string[i++] = (char)c;
		length++;
	}
	if (i + 1 != size) {
		char *_string = realloc(string, length + 1);
		if (_string == NULL) {
			free(string);
			return NULL;
		}
		string = _string;
	}
	string[i] = '\0';
	return string;
}
