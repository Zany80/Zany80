#ifndef ZANY_GRAPHICS_H_
#define ZANY_GRAPHICS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct menu_t menu_t;
typedef struct widget_t widget_t;
typedef struct window_t window_t;

typedef enum {
    horizontal, vertical
} group_orientation_t;

menu_t *menu_create(const char *label);
void menu_append(menu_t *menu, widget_t *widget);
void menu_destroy(menu_t *menu);
void menu_destroy_all(menu_t *menu);

window_t *window_create(const char *name);
void window_min_size(window_t *window, float x, float y);
void window_max_size(window_t *window, float x, float y);
void window_auto_size(window_t *window, bool auto_size);
void window_initial_size(window_t *window, float x, float y);
void window_set_titlebar(window_t *window, bool titlebar);
void window_set_pos(window_t *window, float x, float y);
void window_append(window_t *window, widget_t *widget);
void window_remove(window_t *window, widget_t *widget);
/// Adds the specified menu to the window's menu bar
void window_append_menu(window_t *window, menu_t *menu);
void window_remove_menu(window_t *window, menu_t *menu);
void window_get_pos(window_t *window, float *x, float *y); 
void window_get_size(window_t *window, float *x, float *y);
window_t *get_root();
/// Removes all widgets from the window. If kill_kids is set, all widgets are
/// destroyed as well.
void window_clear(window_t *window, bool kill_kids);
void window_destroy(window_t *window);
bool window_is_minimized(window_t *window);
/// Add a window to an internal list of windows to render every frame.
/// For plugins which just use window_render as their perpetual->frame callback,
/// this allows for removing the need for perpetual status (thus simplifying the
/// plugin further)
void window_register(window_t *);
void window_unregister(window_t *);

widget_t *button_create(const char *label, void(*handler)());
widget_t *menuitem_create(const char *label, void(*handler)());
widget_t *checkbox_create(const char *label, bool *value, void(*handler)());
widget_t *radio_create(const char *label, int *current, int index, void(*handler)(int index));
widget_t *label_create(const char *label);
widget_t *editor_create();
widget_t *input_create(const char *label, size_t capacity, void (*handler)(widget_t *input));
/// RGBA buffer, buf MUST have a size of at least width*height*4
widget_t *image_create(uint8_t *buf, size_t width, size_t height);
/// RGBA buffer, buf MUST have a size of at least width*height*4. buf must remain alive
/// until this widget is destroyed!
widget_t *framebuffer_create(uint8_t *buf, size_t width, size_t height);
/// This SCALES the image, it does NOT affect framebuffer size!
void image_set_size(widget_t *image, float w, float h);
widget_t *customwidget_create(void (*render)());
void widget_set_label(widget_t *widget, const char *label);
void widget_set_visible(widget_t *widget, bool visible);
void widget_destroy(widget_t *widget);
widget_t* label_set_wrapped(widget_t *widget, bool wrapped);
void input_set_text(widget_t *widget, const char *text);
/// The returned string is owned by the widget. This mustn't be used on editors.
const char *input_get_text(widget_t *widget);
void input_set_password(widget_t *widget, bool pass);

widget_t *group_create();
void group_add(widget_t *group, widget_t *widget);
void group_remove(widget_t *group, widget_t *widget);
void group_clear(widget_t *group, bool destroy_contained);
void group_setorientation(widget_t *group, group_orientation_t orientation);

widget_t *submenu_create(menu_t *menu);
void submenu_set_collapsed(widget_t *widget, bool collapsed);

void render_window(window_t *window);

/// Adds the specified menu to the main menu bar
void append_main_menu(menu_t *menu);

#ifndef TEXT_EDITOR
typedef void TextEditor;
#endif

/// renders all registered windows
void render_windows();

widget_t *widget_new(const char *label);
void editor_destroy(TextEditor *editor);
void editor_set_text(widget_t *widget, const char *text);
// Returns the entered text. The caller owns the returned string.
// If len is non-null, size is written to the specified address.
char *editor_get_text(widget_t *widget, size_t *len);
void image_free(widget_t *widget);

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
    bool pw;
} input_t;

typedef struct {
    void (*render)();
} customwidget_t;

typedef enum {
    // menu_item is the same as button, except instead of having a border it
    // fills up all space available to it
    WIDGET_TYPE_BUTTON, WIDGET_TYPE_MENU_ITEM, WIDGET_TYPE_CHECKBOX, WIDGET_TYPE_RADIO, WIDGET_TYPE_LABEL, 
    WIDGET_TYPE_GROUP, WIDGET_TYPE_SUBMENU,
    WIDGET_TYPE_EDITOR, WIDGET_TYPE_INPUT,
    WIDGET_TYPE_IMAGE,
    WIDGET_TYPE_CUSTOM
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
    /// If not negative 1, the widget's width is overridden to this value
    int width;
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

#endif
