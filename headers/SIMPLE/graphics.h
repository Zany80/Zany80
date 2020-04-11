#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/dllports.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct menu_t menu_t;
typedef struct widget_t widget_t;
typedef struct window_t window_t;
typedef struct widget_t widget_t;

typedef enum {
    horizontal, vertical
} group_orientation_t;

SIMPLE_DLL menu_t *menu_create(const char *label);
SIMPLE_DLL void menu_append(menu_t *menu, widget_t *widget);
SIMPLE_DLL void menu_destroy(menu_t *menu);
SIMPLE_DLL void menu_destroy_all(menu_t *menu);

SIMPLE_DLL window_t *window_create(const char *name);
SIMPLE_DLL void window_min_size(window_t *window, float x, float y);
SIMPLE_DLL void window_max_size(window_t *window, float x, float y);
SIMPLE_DLL void window_auto_size(window_t *window, bool auto_size);
SIMPLE_DLL void window_initial_size(window_t *window, float x, float y);
SIMPLE_DLL void window_set_titlebar(window_t *window, bool titlebar);
SIMPLE_DLL void window_set_pos(window_t *window, float x, float y);
SIMPLE_DLL void window_append(window_t *window, widget_t *widget);
SIMPLE_DLL void window_remove(window_t *window, widget_t *widget);
/// Adds the specified menu to the window's menu bar
SIMPLE_DLL void window_append_menu(window_t *window, menu_t *menu);
SIMPLE_DLL void window_remove_menu(window_t *window, menu_t *menu);
SIMPLE_DLL void window_get_pos(window_t *window, float *x, float *y); 
SIMPLE_DLL void window_get_size(window_t *window, float *x, float *y);
SIMPLE_DLL window_t *get_root();
SIMPLE_DLL void window_destroy(window_t *window);
SIMPLE_DLL bool window_is_minimized(window_t *window);
/// Add a window to an internal list of windows to render every frame.
/// For plugins which just use window_render as their perpetual->frame callback,
/// this allows for removing the need for perpetual status (thus simplifying the
/// plugin further)
SIMPLE_DLL void window_register(window_t *);
SIMPLE_DLL void window_unregister(window_t *);

SIMPLE_DLL widget_t *button_create(const char *label, void(*handler)());
SIMPLE_DLL widget_t *menuitem_create(const char *label, void(*handler)());
SIMPLE_DLL widget_t *checkbox_create(const char *label, bool *value, void(*handler)());
SIMPLE_DLL widget_t *radio_create(const char *label, int *current, int index, void(*handler)(int index));
SIMPLE_DLL widget_t *label_create(const char *label);
SIMPLE_DLL widget_t *editor_create(const char *label);
SIMPLE_DLL widget_t *input_create(const char *label, size_t capacity, void (*handler)(widget_t *input));
/// RGBA buffer, buf MUST have a size of at least width*height*4
SIMPLE_DLL widget_t *image_create(uint8_t *buf, size_t width, size_t height);
/// RGBA buffer, buf MUST have a size of at least width*height*4. buf must remain alive
/// until this widget is destroyed!
SIMPLE_DLL widget_t *framebuffer_create(uint8_t *buf, size_t width, size_t height);
/// This SCALES the image, it does NOT affect framebuffer size!
SIMPLE_DLL void image_set_size(widget_t *image, float w, float h);
SIMPLE_DLL widget_t *customwidget_create(void (*render)());
SIMPLE_DLL void widget_set_label(widget_t *widget, const char *label);
SIMPLE_DLL void widget_set_visible(widget_t *widget, bool visible);
SIMPLE_DLL void widget_destroy(widget_t *widget);
SIMPLE_DLL widget_t* label_set_wrapped(widget_t *widget, bool wrapped);
SIMPLE_DLL void input_set_text(widget_t *widget, const char *text);
/// Allocated via malloc, MUST BE free()d BY CALLER
SIMPLE_DLL char *input_get_text(widget_t *widget);
SIMPLE_DLL void input_set_password(widget_t *widget, bool pass);

SIMPLE_DLL widget_t *group_create();
SIMPLE_DLL void group_add(widget_t *group, widget_t *widget);
SIMPLE_DLL void group_remove(widget_t *group, widget_t *widget);
SIMPLE_DLL void group_clear(widget_t *group, bool destroy_contained);
SIMPLE_DLL void group_setorientation(widget_t *group, group_orientation_t orientation);

SIMPLE_DLL widget_t *submenu_create(menu_t *menu);
SIMPLE_DLL void submenu_set_collapsed(widget_t *widget, bool collapsed);

SIMPLE_DLL void render_window(window_t *window);

/// Adds the specified menu to the main menu bar
SIMPLE_DLL void append_main_menu(menu_t *menu);

#ifdef __cplusplus
}
#endif
