#include <SIMPLE/Plugin.h>
#include <string.h>

bool is_type(const char *type) {
	return !strcmp(type, "Toolchain") || !strcmp(type, "Compiler") || !strcmp(type, "DragonFruit");
}

plugin_version_t version_info = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.str = "alpha-1.0.0"
};

static toolchain_conversion_t conversions[2] = {
	{
		.source_ext = ".d",
		.target_ext = ".asm"
	},
	{
		.source_ext = ".df",
		.target_ext = ".asm"
	}
};

list_t *get_compiler_targets() {
	list_t *target_list = create_list();
	list_add(target_list, &conversions[0]);
	list_add(target_list, &conversions[1]);
	return target_list;
}

int compile(list_t *sources, const char *target, char **buffer);
toolchain_plugin_t toolchain_interface = {
	.get_conversions = get_compiler_targets,
	.convert = compile
};

plugin_t plugin_interface = {
	.supports = is_type,
	.name = "DragonFruit Compiler",
	.version = &version_info,
	.toolchain = &toolchain_interface
};

PLUGIN_EXPORT void init() {
	
}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin_interface;
}

PLUGIN_EXPORT void cleanup() {
	
}
