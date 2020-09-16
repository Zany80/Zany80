#define _XOPEN_SOURCE 500
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "graphics.h"
#include "stb/stb_ds.h"

static window_t root = {
	.widgets = NULL,
	.menus = NULL,
	.titlebar = false,
	.name = "##SIMPLERoot",
	.auto_size = false,
	.initialX = -1, .initialY = -1,
	.maxX = -1, .maxY = -1,
	.minX = -1, .minY = -1,
	.absX = -1, .absY = -1,
};

window_t *window_create(const char *name) {
	window_t *w = malloc(sizeof(window_t));
	w->initialX = w->initialY = w->maxX = w->maxY = w->minX = w->minY = -1;
	w->name = name;
	w->widgets = NULL;
	w->menus = NULL;
	w->absX = w->absY = -1;
	w->titlebar = true;
	w->minimized = false;
	w->auto_size = false;
	return w;
}

void window_auto_size(window_t *window, bool auto_size) {
	window->auto_size = auto_size;
}

void window_min_size(window_t *window, float x, float y) {
	window->minX = x;
	window->minY = y;
}

void window_max_size(window_t *window, float x, float y) {
	window->maxX = x;
	window->maxY = y;
}

void window_initial_size(window_t *window, float x, float y) {
	window->initialX = x;
	window->initialY = y;
}

void window_set_titlebar(window_t *window, bool titlebar) {
	window->titlebar = titlebar;
}

void window_set_pos(window_t *window, float x, float y) {
	window->absX = x;
	window->absY = y;
}

void window_clear(window_t *window, bool f) {
	if (f) {
		for (size_t i = 0; i < arrlenu(window->widgets); i++) {
			widget_destroy(window->widgets[i]);
		}
	}
	stbds_arrfree(window->widgets);
	window->widgets = NULL;
}

void window_append(window_t *window, widget_t *widget) {
	stbds_arrpush(window->widgets, widget);
}

void window_append_menu(window_t *window, menu_t *menu) {
	stbds_arrpush(window->menus, menu);
}

void window_remove_menu(window_t *window, menu_t *menu) {
	int index = stbds_arrfind(window->menus, menu);
	if (index != -1) {
		stbds_arrdel(window->menus, index);
	}
}

void window_remove(window_t *window, widget_t *widget) {
	int index = stbds_arrfind(window->widgets, widget);
	if (index != -1) {
		stbds_arrdel(window->widgets, index);
	}
}

window_t *get_root() {
	return &root;
}

void window_destroy(window_t *window) {
	stbds_arrfree(window->menus);
	stbds_arrfree(window->widgets);
	if (window != get_root()) {
		free(window);
	}
}

bool window_is_minimized(window_t *window) {
	return window->minimized;
}

menu_t *menu_create(const char *name) {
	menu_t *m = malloc(sizeof(menu_t));
	m->widgets = NULL;
	m->label = strdup(name);
	return m;
}

widget_t *submenu_create(menu_t *menu) {
	widget_t *w = widget_new(NULL);
	w->type = WIDGET_TYPE_SUBMENU;
	w->menu.menu = menu;
	w->menu.collapsed = false;
	return w;
}

void submenu_set_collapsed(widget_t *widget, bool collapsed) {
	widget->menu.collapsed = collapsed;
}

void menu_append(menu_t *menu, widget_t *widget) {
	stbds_arrpush(menu->widgets, widget);
}

void menu_destroy(menu_t *menu) {
	stbds_arrfree(menu->widgets);
	free(menu->label);
	free(menu);
}

void menu_destroy_all(menu_t *menu) {
	for (size_t i = 0; i < arrlenu(menu->widgets); i++) {
		widget_destroy(menu->widgets[i]);
	}
}

widget_t *widget_new(const char *label) {
	widget_t *w = malloc(sizeof(widget_t));
	w->label = label ? strdup(label) : NULL;
	w->visible = true;
	w->width = -1;
	return w;
}

widget_t *button_create(const char *label, void(*handler)()) {
	widget_t *w = widget_new(label);
	w->type = WIDGET_TYPE_BUTTON;
	w->button.handler = handler;
	return w;
}

widget_t *menuitem_create(const char *label, void(*handler)()) {
	widget_t *w = button_create(label, handler);
	w->type = WIDGET_TYPE_MENU_ITEM;
	return w;
}

widget_t *checkbox_create(const char *label, bool *value, void(*handler)()) {
	widget_t *w = widget_new(label);
	w->type = WIDGET_TYPE_CHECKBOX;
	w->checkbox.handler = handler;
	w->checkbox.value = value;
	return w;
}

widget_t *radio_create(const char *label, int *current, int index, void (*handler)(int index)) {
	widget_t *w = widget_new(label);
	w->type = WIDGET_TYPE_RADIO;
	w->radio.current = current;
	w->radio.index = index;
	w->radio.handler = handler;
	return w;
}

uint8_t extract_nibble(char c) {
	if (c <= '9' && c >= '0') {
		return c - '0';
	}
	else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

uint8_t extract_hex8(const char *hex) {
	return (extract_nibble(hex[0]) << 4) | extract_nibble(hex[1]);
}

uint32_t extract_color(const char *hex) {
	uint8_t r = extract_hex8(hex);
	uint8_t g = extract_hex8(hex + 2);
	uint8_t b = extract_hex8(hex + 4);
	return r | (g << 8) | (b << 16) | (0xFF << 24);
}

widget_t *label_create(const char *_label) {
	widget_t *w = widget_new(_label);
	w->type = WIDGET_TYPE_LABEL;
	w->_label.wrapped = false;
	w->_label.color = 0xFFFFFFFF;
	w->_label.next = NULL;
	widget_set_label(w, _label);
	return w;
}

widget_t *input_create(const char *label, size_t capacity, void (*handler)(widget_t *widget)) {
	widget_t *w = widget_new(label);
	w->type = WIDGET_TYPE_INPUT;
	w->input.capacity = capacity;
	w->input.handler = handler;
	w->input.buf = malloc(capacity);
	w->input.pw = false;
	strcpy(w->input.buf, "");
	return w;
}

widget_t *customwidget_create(void (*handler)()) {
	widget_t *w = widget_new(NULL);
	w->type = WIDGET_TYPE_CUSTOM;
	w->custom.render = handler;
	return w;
}

void input_set_password(widget_t *w, bool pw) {
	w->input.pw = pw;
}

char *input_get_text(widget_t *widget) {
	if (widget->type == WIDGET_TYPE_EDITOR) {
		return editor_get_text(widget);
	}
	else {
		return strdup(widget->input.buf);
	}
}

void input_set_text(widget_t *widget, const char *text) {
	if (widget->type == WIDGET_TYPE_EDITOR) {
		editor_set_text(widget, text);
	}
	else {
		strncpy(widget->input.buf, text, widget->input.capacity);
	}
}

widget_t* label_set_wrapped(widget_t *widget, bool wrapped) {
	if (widget->type == WIDGET_TYPE_LABEL) {
		widget->_label.wrapped = wrapped;
		if (widget->_label.next != NULL) {
			label_set_wrapped(widget->_label.next, wrapped);
		}
	}
	return widget;
}

void widget_set_label(widget_t *widget, const char *label) {
	if (widget->label) {
		free(widget->label);
	}
	if (widget->_label.next) {
		widget_destroy(widget->_label.next);
	}
	widget->label = label ? strdup(label) : NULL;
	if (!widget->label) {
		return;
	}
	char c;
	enum {
		normal, color_match
	} state = normal;
	int cbegin = 0;
	int index = 0;
	while ((c = label[index++]) != 0) {
		if (c == '[' && state == normal) {
			state = color_match;
			cbegin = index;
		}
		if (c == ']' && state == color_match) {
			state = normal;
			if (index - cbegin == 7) {
				bool valid_color = true;
				for (int i = 0; i < 6; i++) {
					int index = cbegin + i;
					if (!((label[index] >= '0' && label[index] <= '9') || (label[index] >= 'A' && label[index] <= 'F'))) {
						valid_color = false;
						break;
					}
				}
				if (valid_color) {
					widget->label[cbegin - 1] = 0;
					widget->_label.next = label_create(label + index);
					widget->_label.next->_label.color = extract_color(label + cbegin);
					break;
				}
			}
			else {
				printf("Index - cbegin = %d; %s\n",index - cbegin, label + index);
			}
		}
	}
}

void widget_set_visible(widget_t *widget, bool visible) {
	widget->visible = visible;
}

void widget_destroy(widget_t *widget) {
	switch (widget->type) {
		case WIDGET_TYPE_GROUP:
			stbds_arrfree(widget->_group.widgets);
			break;
		case WIDGET_TYPE_EDITOR:
			editor_destroy(widget->editor.editor);
			break;
		case WIDGET_TYPE_INPUT:
			free(widget->input.buf);
			break;
		case WIDGET_TYPE_IMAGE:
			image_free(widget);
			break;
		case WIDGET_TYPE_LABEL:
			if (widget->_label.next) {
				widget_destroy(widget->_label.next);
			}
			break;
		default:
			break;
	}
	if (widget->label) {
		free(widget->label);
	}
	free(widget);
}

void append_main_menu(menu_t *menu) {
	window_append_menu(get_root(), menu);
}

widget_t *group_create() {
	widget_t *g = widget_new(NULL);
	g->type = WIDGET_TYPE_GROUP;
	g->_group.widgets = NULL;
	g->_group.orientation = vertical;
	return g;
}

void group_add(widget_t *group, widget_t *widget) {
	stbds_arrpush(group->_group.widgets, widget);
}

void group_remove(widget_t *group, widget_t *widget) {
	int index = stbds_arrfind(group->_group.widgets, widget);
	if (index != -1) {
		stbds_arrdel(group->_group.widgets, index);
	}
}

void group_clear(widget_t *group, bool f) {
	if (f) {
		for (size_t i = 0; i < arrlenu(group->_group.widgets); i++) {
			widget_destroy(group->_group.widgets[i]);
		}
	}
	stbds_arrfree(group->_group.widgets);
	group->_group.widgets = NULL;
}

void group_setorientation(widget_t *group, group_orientation_t orientation) {
	group->_group.orientation = orientation;
}

void image_set_size(widget_t *image, float w, float h) {
	image->image.visible_width = w;
	image->image.visible_height = h;
}

static window_t **windows;

void window_register(window_t *w) {
	stbds_arrpush(windows, w);
}

void window_unregister(window_t *w) {
	int index = stbds_arrfind(windows, w);
	if (index != -1) {
		stbds_arrdel(windows, index);
	}
}

void render_windows() {
	for (size_t i = 0; i < arrlenu(windows); i++) {
		render_window(windows[i]);
	}
}
