#include "stringop.h"
#include <stdlib.h>
#include <string.h>

/* Note: This returns 8 characters for trimmed_start per tab character. */
char *strip_whitespace(char *_str, int *trimmed_start) {
	*trimmed_start = 0;
	if (*_str == '\0')
		return _str;
	char *strold = _str;
	while (*_str == ' ' || *_str == '\t') {
		if (*_str == '\t') {
			*trimmed_start += 8;
		} else {
			*trimmed_start += 1;
		}
		_str++;
	}
	char *str = malloc(strlen(_str) + 1);
	strcpy(str, _str);
	free(strold);
	int i;
	for (i = 0; str[i] != '\0'; ++i);
	do {
		i--;
	} while (i >= 0 && (str[i] == ' ' || str[i] == '\t'));
	str[i + 1] = '\0';
	return str;
}
