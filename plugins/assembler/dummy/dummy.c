#include <Zany80/Plugin.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int convert(list_t *sources, const char *target, char **buffer) {
	*buffer = malloc(1024 * 1024);
	snprintf(*buffer, 1024 * 1024 - 1, "Dummy assembler told to assemble into '%s' the following files: ", target);
	for (int i = 0; i < sources->length; i++) {
		strcat(strcat(*buffer, (char*)(sources->items[i])), "\n");
	}
	return 0;
}

static toolchain_conversion_t conversions[] = {
	{
		.source_ext = ".asm",
		.target_ext = ".o"
	},
	{
		.source_ext = ".o",
		.target_ext = ".zany"
	}
};

list_t *get_conversions() {
	list_t *_conversions = create_list();
	list_add(_conversions, &conversions[0]);
	list_add(_conversions, &conversions[1]);
	return _conversions;
}

bool supports(const char *feature) {
	return !strcmp(feature, "Toolchain") || !strcmp(feature, "Assembler");
}

plugin_version_t version = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.str = "Dummy 1.0.0"
};

toolchain_plugin_t toolchain = {
	.get_conversions = get_conversions,
	.convert = convert
};

plugin_t plugin = {
	.name = "Dummy Assembler",
	.supports = supports,
	.version = &version,
	.toolchain = &toolchain
};

PLUGIN_EXPORT void init() {}
PLUGIN_EXPORT void cleanup() {}
PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin;
}
