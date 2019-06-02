#pragma once

#ifdef __cplusplus
#include <TextEditor.h>
extern "C" {
#else
typedef void TextEditor;
#endif

widget_t *widget_new(const char *label);
void editor_destroy(TextEditor *editor);
void editor_set_text(widget_t *widget, const char *text);
char *editor_get_text(widget_t *widget);

#ifdef __cplusplus
}
#endif

#include <stddef.h>

typedef struct {
    void (*handler)();
} button_t;

typedef struct {
    bool *value;
    void (*handler)();
} checkbox_t;

typedef struct {
	int *current;
	int index;
	void (*handler)(int index);
} radio_t;

typedef struct {
    bool wrapped;
} label_t;

typedef struct {
    widget_t **widgets;
    group_orientation_t orientation;
} group_t;

typedef struct {
	TextEditor *editor;
} editor_t;

typedef struct {
	char *buf;
	size_t capacity;
	void (*handler)(widget_t *widget);
} input_t;

typedef struct {
    void (*render)();
} customwidget_t;

typedef enum {
    // menu_item is the same as button, except instead of having a border it
    // fills up all space available to it
    button, menu_item, checkbox, radio, label, 
    group, submenu,
    editor, input, 
    custom
} widget_type;

struct widget_t {
    widget_type type;
    const char *label;
    union {
        button_t button;
        checkbox_t checkbox;
        radio_t radio;
        label_t _label;
        group_t _group;
        editor_t editor;
        input_t input;
        customwidget_t custom;
        menu_t *menu;
    };
    bool visible;
};

struct menu_t {
    widget_t **widgets;
    const char *label;
};

struct window_t {
    menu_t **menus;
    widget_t **widgets;
    const char *name;
    float minX, minY, maxX, maxY;
    float initialX, initialY;
    float absX, absY;
    bool titlebar, minimized;
};
