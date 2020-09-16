#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// a/b/sej

bool make_containing_folder(const char *const path) {
	int last_slash = strlen(path);
	while (path[last_slash - 1] != '/') {
		last_slash -= 1;
		if (last_slash == 0) {
			fprintf(stderr, "Path appears to be relative to current dir, not making any folder.\n");
			return true;
		}
	}
	char *buffer = malloc(last_slash + 1);
	strncpy(buffer, path, last_slash);
	const int result = mkdir(buffer, S_IRWXU | S_IRWXG | S_IROTH);
	if (errno == 0) {
		fprintf(stderr, "Created %s\n", buffer);
	}
	else if (errno != EEXIST) {
		fprintf(stderr, "Failed to create %s\n", buffer);
	}
	free(buffer);
	return result == 0 || errno == EEXIST;
}

int main(int argc, const char **argv) {
	if (argc != 4) {
		printf("Usage: %s SOURCE DESTINATION HEADER\n", argv[0]);
		return 1;
	}
	FILE *source = fopen(argv[1], "rb");
	if (!source) {
		perror("Error opening source file");
		return 1;
	}
	if (!make_containing_folder(argv[2])) {
	   fprintf(stderr, "Error creating output's containing folder!\n");
	   return 1;
	}
	FILE *destination = fopen(argv[2], "wb");
	if (!destination) {
		perror("Error opening destination file");
		fclose(source);
		return 1;
	}
	FILE *header = fopen(argv[3], "wb");
	if (!header) {
		perror("Error opening header file");
		fclose(source);
		fclose(destination);
		return 1;
	}
	int return_code = 1;

	if (fseek(source, 0, SEEK_END) != 0) {
		puts("Seek failed!");
		goto cleanup;
	}
	long length = ftell(source);
	if (length == -1) {
		puts("Failed to determine source file size!");
		goto cleanup;
	}
	rewind(source);
	char *buf = malloc(length);
	if (!buf) {
		puts("out of memory!");
		goto cleanup;
	}
	if (fread(buf, 1, length, source) != (size_t)length) {
		puts("Failed to read source file into buffer!");
		goto cleanup;
	}

	if (fprintf(destination, "const char z80_tab[%lu] = {", length + 1) < 0) {
		puts("Failed to print to file!");
		goto cleanup;
	}
	if (fprintf(header, "#ifndef Z80_TABLE\n#define Z80_TABLE\n\nextern const char z80_tab[%lu];\n\n#endif\n", length + 1) < 0) {
		puts("Failed to write header!");
		goto cleanup;
	}
	for (long i = 0; i < length; i++) {
		if (fprintf(destination, "%s0x%02x,", i % 8 == 0 ? "\n\t" : " ", buf[i]) < 0) {
			puts("Failed to print to file!");
			goto cleanup;
		}
	}

	if (fwrite("\n\t0,\n};\n", 1, 8, destination) != 8) {
		puts("Failed to write to file!");
		goto cleanup;
	}
	return_code = 0;
cleanup:
	if (fflush(destination) != 0) {
		puts("Failed to flush file!");
		return_code = 1;
	}
	if (fflush(header) != 0) {
		puts("Failed to flush header file!");
		return_code = 1;
	}
	if (fclose(destination) != 0) {
		puts("Failed to close output file!");
		return_code = 1;
	}
	if (fclose(header) != 0) {
		puts("Failed to close header file!");
		return_code = 1;
	}
	if (fclose(source) != 0) {
		puts("Failed to close input file!");
		return_code = 1;
	}

	return return_code;
}
