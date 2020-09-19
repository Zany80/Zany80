#include "editor.h"
#include "serial.h"
#include "graphics.h"
#include "scas.h"

#include <stdio.h>
#include <stdlib.h>

static window_t *window;
static widget_t *label, *editor;
static menu_t *menu;
static char *tmp_buf;

static void editor_assemble() {
	size_t len;
	const char *source = editor_get_text(editor, &len);
	FILE *infile = fmemopen((void*)source, len, "r");
	FILE *outfile = fmemopen(tmp_buf, 16 * 1024 * 1024, "w");
	widget_set_label(label, scas_assemble(infile, outfile) ? "[00FF00]Built successfully!" : "[FF0000]Failed to assemble!");
}

static void editor_reset() {
	widget_set_label(label, "[FF0FA0]No build attempted");
}

void editor_init() {
	window = window_create("Editor");
	window_register(window);
	window_append(window, label = label_set_wrapped(label_create("[FF0FA0]No build attempted"), false));
	window_append(window, editor = editor_create());
	window_min_size(window, 400, 600);
	menu = menu_create("Build");
	window_append_menu(window, menu);
	menu_append(menu, menuitem_create("Assemble", editor_assemble));
	menu_append(menu, menuitem_create("Clear errors", editor_reset));
	tmp_buf = malloc(16 * 1024 * 1024);
}

void editor_deinit() {
	if (editor) {
		window_unregister(window);
		window_clear(window, true);
		window_destroy(window);
		editor = NULL;
		menu_destroy_all(menu);
		menu_destroy(menu);
		menu = NULL;
		free(tmp_buf);
	}
}
