#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SIMPLE/Plugin.h>

plugin_t *get_interface();

int main(int argc, char **argv) {
	argc--;
	argv++;
	if (argc < 2) {
		puts("Usage: dragonfruit source1 .. sourceN target");
		return 1;
	}
	toolchain_plugin_t *compiler = get_interface()->toolchain;
	list_t *source = create_list();
	for (int i = 0; i < argc - 1; i++) {
		list_add(source, argv[i]);
	}
	char *output_buffer;
	int r = compiler->convert(source, argv[argc - 1], &output_buffer);
	list_free(source);
	printf("%s", output_buffer);
	size_t s = strlen(output_buffer);
	if (s > 0 && output_buffer[s - 1] != '\n') {
		putchar('\n');
	}
	free(output_buffer);
	return r;
}
