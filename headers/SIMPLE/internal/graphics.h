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
void image_free(widget_t *widget);

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

/*
 * labels are split into sections using a simple linked-list. A simple color
 * markup system is used: any square brackets ([]) containing a valid hex number
 * (e.g. [34F723]) is interpreted as rgb, which causes the end of the current
 * section and a new section using the listed color to be created. If no valid
 * hex is found inside the square brackets or no matching ] is located, it is
 * assumed that no color change is intended and the string analysis continues.
 */
typedef struct {
    // whether current text should be automatically wrapped
    bool wrapped;
    // color of current section
    uint32_t color;
    // next section of text
    widget_t *next;
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
    image,
    custom
} widget_type;

typedef struct {
    menu_t *menu;
    bool collapsed;
} submenu_t;

typedef struct {
	uint8_t *buf;
	size_t width, height;
	bool stream;
	/// implementation-independent texture ID.
	/// For the current Oryol/ImGui hybrid implementation, this is a Gfx ID 
	/// / ImTextureID pair
	void *id;
	/// visible size. Scaled from original size to this when rendered.
	float visible_width, visible_height;
} image_t;

struct widget_t {
    widget_type type;
    char *label;
    union {
        button_t button;
        checkbox_t checkbox;
        radio_t radio;
        label_t _label;
        group_t _group;
        editor_t editor;
        input_t input;
        customwidget_t custom;
        submenu_t menu;
        image_t image;
    };
    bool visible;
};

struct menu_t {
    widget_t **widgets;
    char *label;
};

struct window_t {
    menu_t **menus;
    widget_t **widgets;
    const char *name;
    float minX, minY, maxX, maxY;
    float initialX, initialY;
    float absX, absY;
    bool titlebar, minimized;
    bool auto_size;
};
