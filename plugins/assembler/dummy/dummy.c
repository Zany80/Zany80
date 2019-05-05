#include <Zany80/Plugin.h>

int convert(list_t *sources, const char *target, char **buffer) {
	sources;
	target;
	buffer;
	return 0;
}

static const toolchain_conversion_t conversions[] = {
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
	list_t *conversions = create_list();
	list_add(conversions, &conversions[0]);
	list_add(conversions, &conversions[1]);
	return conversions;
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