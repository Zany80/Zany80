#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SIMPLE/scas/list.h>
#include <dirent.h>

list_t *simple_read_directory(const char *path) {
	DIR *dir = opendir(path);
	list_t *contents = NULL;
	if (dir != NULL) {
		contents = create_list();
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.')
				continue;
			char *name = malloc(strlen(entry->d_name) + 1);
			strcpy(name, entry->d_name);
			list_add(contents, name);
		}
		closedir(dir);
	}
	else {
		fprintf(stderr, "Error opening directory: %s\n", path);
	}
    return contents;
}
