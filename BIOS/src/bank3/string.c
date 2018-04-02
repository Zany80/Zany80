#include <stdbool.h>

void strcpy(char * dest, const char * src){
	int index;
	for (index = 0; src[index] != 0; index++) {
		dest[index] = src[index];
	}
}

bool strequal(const char *s1, const char *s2) {
	int index;
	for (index = 0; s1[index] != 0 && s2[index] != 0; index++) {
		if (s1[index] != s2[index]) {
			return false;
		}
	}
	return s1[index] == s2[index];
}
