#include <stdio.h>
#include <string.h>

#include "SIMPLE/API/graphics.h"
#include "SIMPLE/3rd-party/stretchy_buffer.h"
#include "SIMPLE/internal/graphics.h"

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

void window_append(window_t *window, widget_t *widget) {
    sb_push(window->widgets, widget);
}

void window_append_menu(window_t *window, menu_t *menu) {
    sb_push(window->menus, menu);
}

void window_remove_menu(window_t *window, menu_t *menu) {
    sb_remove((void***)&window->menus, menu);
}

void window_remove(window_t *window, widget_t *widget) {
    sb_remove((void***)&window->widgets, widget);
}

window_t *get_root() {
    return &root;
}

void window_destroy(window_t *window) {
    sb_free(window->menus);
    sb_free(window->widgets);
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
	w->type = submenu;
	w->menu.menu = menu;
    w->menu.collapsed = false;
	return w;
}

void submenu_set_collapsed(widget_t *widget, bool collapsed) {
    widget->menu.collapsed = collapsed;
}

void menu_append(menu_t *menu, widget_t *widget) {
    sb_push(menu->widgets, widget);
}

void menu_destroy(menu_t *menu) {
    sb_free(menu->widgets);
    free(menu->label);
    free(menu);
}

void menu_destroy_all(menu_t *menu) {
	for (int i = 0; i < sb_count(menu->widgets); i++) {
		widget_destroy(menu->widgets[i]);
	}
}

widget_t *widget_new(const char *label) {
    widget_t *w = malloc(sizeof(widget_t));
    w->label = label ? strdup(label) : NULL;
    w->visible = true;
    return w;
}

widget_t *button_create(const char *label, void(*handler)()) {
    widget_t *w = widget_new(label);
    w->type = button;
    w->button.handler = handler;
    return w;
}

widget_t *menuitem_create(const char *label, void(*handler)()) {
    widget_t *w = button_create(label, handler);
    w->type = menu_item;
    return w;
}

widget_t *checkbox_create(const char *label, bool *value, void(*handler)()) {
    widget_t *w = widget_new(label);
    w->type = checkbox;
    w->checkbox.handler = handler;
    w->checkbox.value = value;
    return w;
}

widget_t *radio_create(const char *label, int *current, int index, void (*handler)(int index)) {
	widget_t *w = widget_new(label);
	w->type = radio;
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
    w->type = label;
    w->_label.wrapped = false;
    w->_label.color = 0xFFFFFFFF;
    w->_label.next = NULL;
    if (_label != NULL) {
        char c;
        enum {
            normal, color_match
        } state = normal;
        int cbegin;
        int index = 0;
        while ((c = _label[index++]) != 0) {
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
                        if (!((_label[index] >= '0' && _label[index] <= '9') || (_label[index] >= 'A' && _label[index] <= 'F'))) {
                            valid_color = false;
                            break;
                        }
                    }
                    if (valid_color) {
                        w->label[cbegin - 1] = 0;
                        w->_label.next = label_create(_label + index);
                        w->_label.next->_label.color = extract_color(_label + cbegin);
                        break;
                    }
                }
                else {
                    printf("Index - cbegin = %d; %s\n",index - cbegin, _label + index);
                }
            }
        }
    }
    return w;
}

widget_t *input_create(const char *label, size_t capacity, void (*handler)(widget_t *widget)) {
	widget_t *w = widget_new(label);
	w->type = input;
	w->input.capacity = capacity;
	w->input.handler = handler;
	w->input.buf = malloc(capacity);
	strcpy(w->input.buf, "");
	return w;
}

widget_t *customwidget_create(void (*handler)()) {
    widget_t *w = widget_new(NULL);
    w->type = custom;
    w->custom.render = handler;
    return w;
}

char *input_get_text(widget_t *widget) {
	if (widget->type == editor) {
		return editor_get_text(widget);
	}
	else {
		return strdup(widget->input.buf);
	}
}

void input_set_text(widget_t *widget, const char *text) {
	if (widget->type == editor) {
		editor_set_text(widget, text);
	}
	else {
		strncpy(widget->input.buf, text, widget->input.capacity);
	}
}

widget_t* label_set_wrapped(widget_t *widget, bool wrapped) {
    if (widget->type == label) {
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
    widget->label = label ? strdup(label) : NULL;
}

void widget_set_visible(widget_t *widget, bool visible) {
    widget->visible = visible;
}

void widget_destroy(widget_t *widget) {
	switch (widget->type) {
		case group:
			sb_free(widget->_group.widgets);
			break;
		case editor:
			editor_destroy(widget->editor.editor);
			break;
		case input:
			free(widget->input.buf);
			break;
		case image:
			image_free(widget);
			break;
        case label:
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
    g->type = group;
    g->_group.widgets = NULL;
    g->_group.orientation = vertical;
    return g;
}

void group_add(widget_t *group, widget_t *widget) {
    sb_push(group->_group.widgets, widget);
}

void group_remove(widget_t *group, widget_t *widget) {
    sb_remove((void***)&group->_group.widgets, widget);
}

void group_clear(widget_t *group, bool f) {
    if (f) {
        for (int i = 0; i < sb_count(group->_group.widgets); i++) {
            widget_destroy(group->_group.widgets[i]);
        }
    }
    sb_free(group->_group.widgets);
    group->_group.widgets = NULL;
}

void group_setorientation(widget_t *group, group_orientation_t orientation) {
    group->_group.orientation = orientation;
}

void image_set_size(widget_t *image, float w, float h) {
	image->image.visible_width = w;
	image->image.visible_height = h;
}
