#include <dirent.h>
#include <string.h>
#include <3rd-party/scas/list.h>
#include <stdlib.h>
#include <stdio.h>

list_t *zany_read_directory(const char *path) {
	DIR *dir;
	if ((dir = opendir(path)) != NULL) {
		list_t *contents = create_list();
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.')
				continue;
			char *name = malloc(strlen(entry->d_name) + 1);
			strcpy(name, entry->d_name);
			list_add(contents, name);
		}
		closedir(dir);
		return contents;
	}
	else {
		fprintf(stderr, "Error opening directory: %s\n", path);
		return NULL;
	}
}
