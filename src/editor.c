#include "editor.h"
#include "serial.h"
#include "graphics.h"

window_t *editor;

void editor_init() {
	editor = window_create("Editor");
	window_register(editor);
	window_append(editor, editor_create());
	window_min_size(editor, 400, 600);
}

void editor_deinit() {
	if (editor) {
		window_unregister(editor);
		window_clear(editor, true);
		window_destroy(editor);
		editor = NULL;
	}
}
