#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SIMPLE/scas/list.h>
#include <SIMPLE/internal/dllports.h>
#include <direct.h>
#include <windows.h>

ZANY_DLL list_t *simple_read_directory(const char *path) {
	WIN32_FIND_DATA data;
	char *full_path = malloc(strlen(path) + 3);
	strcpy(full_path, path);
	strcpy(full_path + strlen(path), "\\*");
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
