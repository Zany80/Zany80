#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../internal/dllports.h"

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

ZANY_DLL menu_t *menu_create(const char *label);
ZANY_DLL void menu_append(menu_t *menu, widget_t *widget);
ZANY_DLL void menu_destroy(menu_t *menu);
ZANY_DLL void menu_destroy_all(menu_t *menu);

ZANY_DLL window_t *window_create(const char *name);
ZANY_DLL void window_min_size(window_t *window, float x, float y);
ZANY_DLL void window_max_size(window_t *window, float x, float y);
ZANY_DLL void window_initial_size(window_t *window, float x, float y);
ZANY_DLL void window_set_titlebar(window_t *window, bool titlebar);
ZANY_DLL void window_set_pos(window_t *window, float x, float y);
ZANY_DLL void window_append(window_t *window, widget_t *widget);
ZANY_DLL void window_remove(window_t *window, widget_t *widget);
/// Adds the specified menu to the window's menu bar
ZANY_DLL void window_append_menu(window_t *window, menu_t *menu);
ZANY_DLL void window_remove_menu(window_t *window, menu_t *menu);
ZANY_DLL void window_get_pos(window_t *window, float *x, float *y); 
ZANY_DLL void window_get_size(window_t *window, float *x, float *y);
ZANY_DLL window_t *get_root();
ZANY_DLL void window_destroy(window_t *window);
ZANY_DLL bool window_is_minimized(window_t *window);

ZANY_DLL widget_t *button_create(const char *label, void(*handler)());
ZANY_DLL widget_t *menuitem_create(const char *label, void(*handler)());
ZANY_DLL widget_t *checkbox_create(const char *label, bool *value, void(*handler)());
ZANY_DLL widget_t *radio_create(const char *label, int *current, int index, void(*handler)(int index));
ZANY_DLL widget_t *label_create(const char *label);
ZANY_DLL widget_t *editor_create(const char *label);
ZANY_DLL widget_t *input_create(const char *label, size_t capacity, void (*handler)(widget_t *input));
ZANY_DLL widget_t *customwidget_create(void (*render)());
ZANY_DLL void widget_set_label(widget_t *widget, const char *label);
ZANY_DLL void widget_set_visible(widget_t *widget, bool visible);
ZANY_DLL void widget_destroy(widget_t *widget);
ZANY_DLL void label_set_wrapped(widget_t *widget, bool wrapped);
ZANY_DLL void input_set_text(widget_t *widget, const char *text);
/// Allocated via malloc, MUST BE free()d BY CALLER
ZANY_DLL char *input_get_text(widget_t *widget);

ZANY_DLL widget_t *group_create();
ZANY_DLL void group_add(widget_t *group, widget_t *widget);
ZANY_DLL void group_remove(widget_t *group, widget_t *widget);
/// If f is set, all widgets in the group are destroyed
ZANY_DLL void group_clear(widget_t *group, bool f);
ZANY_DLL void group_setorientation(widget_t *group, group_orientation_t orientation);

ZANY_DLL widget_t *submenu_create(menu_t *menu);

ZANY_DLL void render_window(window_t *window);

/// Adds the specified menu to the main menu bar
ZANY_DLL void append_main_menu(menu_t *menu);

#ifdef __cplusplus
}
#endif
