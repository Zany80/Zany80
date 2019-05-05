#include <direct.h>
#include <string.h>
#include <scas/list.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <Zany80/internal/dllports.h>

ZANY_DLL list_t *zany_read_directory(const char *path) {
	WIN32_FIND_DATA data;
	char *full_path = strdup(path);
	full_path = realloc(full_path, strlen(full_path) + 3);
	strcpy(full_path + strlen(full_path), "\\*");
	HANDLE hFind = FindFirstFile(full_path, &data);
	list_t *contents = NULL;
	if (hFind != INVALID_HANDLE_VALUE) {
		contents = create_list();
		do {
			if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, "..")) {
				list_add(contents, strdup(data.cFileName));
			}
		} while(FindNextFile(hFind, &data));
		FindClose(hFind);
	}
	else {
		fprintf(stderr, "Error opening directory: %s\n", path);
	}
	free(full_path);
	return contents;
}
