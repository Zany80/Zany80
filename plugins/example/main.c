#include <Zany80/Plugin.h>
#include <Zany80/API.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stb_image.h>

menu_t *main_menu;
widget_t *toggle_visible;
bool visible = true;

window_t *window;
widget_t *label;
widget_t *options;
widget_t *zany_icon;

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

void left() {
	widget_set_label(label, "LEEEEEEFT");
}

void right() {
	widget_set_label(label, "RIIIIIGHT");	
}

PLUGIN_EXPORT void init() {
	main_menu = menu_create("Plugin Demo");
	toggle_visible = checkbox_create("Show", &visible, NULL);
	menu_append(main_menu, toggle_visible);
	window_append_menu(get_root(), main_menu);
	window = window_create("Plugin Demo (Adventure Game)");
	window_min_size(window, 200, 150);
	window_initial_size(window, 330, 220);
	label = label_create(
		"This is a simple text adventure written to demonstrate the power and "
		"simplicity of the Zany80 plugin API.\n"
		"\n"
		"Due to time constraints, this is basically empty. It still serves its "
		"purpose as a plugin demo, but it's not actually a game. That will be "
		"fixed eventually.\n"
		"\n"
		"You arrive at a fork in the road.");
	label_set_wrapped(label, true);
	window_append(window, label);
	options = group_create();
	window_append(window, options);
	group_add(options, menuitem_create("Go Left", left));
	group_add(options, menuitem_create("Go Right", right));
	char *root = zany_root_folder();
	char *icon_path = malloc(strlen(root) + 12);
	sprintf(icon_path, "%s/zany80.png", root);
	free(root);
	int width, height;
	unsigned char *image_data = stbi_load(icon_path, &width, &height, NULL, 4);
	if (image_data != NULL) {
		zany_icon = image_create(image_data, width, height);
		window_append(window, zany_icon);
		stbi_image_free(image_data);
	}
	else {
		window_append(window, zany_icon = label_create("Error loading image!"));
	}
}

PLUGIN_EXPORT void cleanup() {
	widget_destroy(zany_icon);
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
