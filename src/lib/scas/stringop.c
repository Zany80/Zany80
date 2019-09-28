#include "scas/stringop.h"
#include "scas/list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "string.h"
#ifdef _WIN32
#include <string.h>
#include <windows.h>
#else
#include <strings.h>
#endif


/* I'm fairly sure every windows toolchain
 * has this feature by now */
#ifndef strcasecmp
int strcasecmp(const char* s1, const char* s2)
{
	while (*s1 && *s2) {
		int c1 = tolower((int)*s1);
		int c2 = tolower((int)*s2);
		if(c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}
	return (int)(*s1 - *s2);
}
#endif

char *strip_comments(char *str) {
	int in_string = 0, in_character = 0;
	int i = 0;
	while (str[i] != '\0') {
		if (str[i] == '"' && !in_character) {
			in_string = !in_string;
		} else if (str[i] == '\'' && !in_string) {
			in_character = !in_character;
		} else if (!in_character && !in_string) {
			if (str[i] == ';') {
				str[i] = '\0';
				break;
			}
		}
		++i;
	}
	return str;
}

list_t *split_string(const char *str, const char *delims) {
	list_t *res = create_list();
	int i, j;
	for (i = 0, j = 0; i < strlen(str) + 1; ++i) {
		if (strchr(delims, str[i]) || i == strlen(str)) {
			if (i - j == 0) {
				continue;
			}
			char *left = malloc(i - j + 1);
			memcpy(left, str + j, i - j);
			left[i - j] = 0;
			list_add(res, left);
			j = i + 1;
			while (j <= strlen(str) && str[j] && strchr(delims, str[j])) {
				j++;
				i++;
			}
		}
	}
	return res;
}

void free_flat_list(list_t *list) {
	int i;
	for (i = 0; i < list->length; ++i) {
		free(list->items[i]);
	}
	list_free(list);
}

char *code_strstr(const char *haystack, const char *needle) {
	/* TODO */
	return strstr(haystack, needle);
}

char *code_strchr(const char *str, char delimiter) {
	int in_string = 0, in_character = 0, in_paren = 0;
	int i = 0;
	while (str[i] != '\0') {
		if (str[i] == '"' && !in_character) {
			in_string = !in_string;
		} else if (str[i] == '\'' && !in_string) {
			in_character = !in_character;
		} else if (str[i] == '(') {
			in_paren++;
		} else if (str[i] == ')' && in_paren) {
			in_paren--;
		} else if (!in_character && !in_string && !in_paren) {
			if (str[i] == delimiter) {
				return (char *)str + i;
			}
		}
		++i;
	}
	return NULL;
}

int unescape_string(char *string) {
	/* TODO: More C string escapes */
	int len = strlen(string);
	int i;
	for (i = 0; string[i]; ++i) {
		if (string[i] == '\\') {
			--len;
			switch (string[i + 1]) {
			case '0':
				string[i] = '\0';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'a':
				string[i] = '\a';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'b':
				string[i] = '\b';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 't':
				string[i] = '\t';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'n':
				string[i] = '\n';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'v':
				string[i] = '\v';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'f':
				string[i] = '\f';
				memmove(string + i, string + i + 1, len - i);
				break;
			case 'r':
				string[i] = '\r';
				memmove(string + i, string + i + 1, len - i);
				break;
			}
		}
	}
	return len;
}
