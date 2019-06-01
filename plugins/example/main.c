#include <Zany80/Plugin.h>
#include <Zany80/API/graphics.h>
#include <string.h>

menu_t *main_menu;
widget_t *toggle_visible;
bool visible = true;

window_t *window;
widget_t *label;
widget_t *options;

void frame(float delta) {
	if (visible) {
		render_window(window);
	}
}

bool supports(const char *type) {
	return (!strcmp(type, "Perpetual"));
}

perpetual_plugin_t perpetual = {
	.frame = frame
};

plugin_t plugin = {
	.name = "Plugin Demo Adventure Game",
	.supports = supports,
	.perpetual = &perpetual
};

PLUGIN_EXPORT void init() {
	main_menu = menu_create("Plugin Demo");
	toggle_visible = checkbox_create("Show", &visible, NULL);
	menu_append(main_menu, toggle_visible);
	window_append_menu(get_root(), main_menu);
	window = window_create("Plugin Demo (Adventure Game)");
	window_min_size(window, 400, 150);
	label = label_create(
		"This is a simple text adventure written to demonstrate the power and "
		"simplicity of the Zany80 plugin API.\n"
		"\n"
		"You arrive at a fork in the road.");
	label_set_wrapped(label, true);
	window_append(window, label);
	options = group_create();
	window_append(window, options);
	
}

PLUGIN_EXPORT void cleanup() {
	window_remove_menu(get_root(), main_menu);
	menu_destroy(main_menu);
	widget_destroy(toggle_visible);
	window_destroy(window);
	widget_destroy(label);
	group_clear(options, true);
	widget_destroy(options);
}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin;
}
