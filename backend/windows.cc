#include <direct.h>
#include <string.h>
#include <list.h>

list_t *read_directory(const char *path) {
	WIN32_FIND_DATA data;
	char full_path[strlen(path) + 3];
	strcpy(full_path, path);
	strcpy(full_path + strlen(path), "\\*");
	HANDLE hFind = FindFirstFile(full_path, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		list_t *contents = create_list();
		do {
			char *name = malloc(strlen(data.cFileName) + 1);
			strcpy(name, data.cFileName);
			list_add(contents, name);
		} while(FindNextFile(hFind, &data));
		FindClose(hFind);
		return contents;
	}
	else {
		fprintf(stderr, "Error opening directory: %s\n", path.AsCStr());
		return NULL;
	}
}
