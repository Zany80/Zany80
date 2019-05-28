#pragma once

typedef struct {
    void (*handler)();
} button_t;

typedef struct {
    bool *value;
    void (*handler)();
} checkbox_t;

typedef struct {
    bool wrapped;
} label_t;

typedef struct {
    widget_t **widgets;
} group_t;

typedef enum {
    // menu_item is the same as button, except instead of having a border it
    // fills up all space available to it
    button, menu_item, checkbox, label, group
} widget_type;

struct widget_t {
    widget_type type;
    const char *label;
    union {
        button_t button;
        checkbox_t checkbox;
        label_t _label;
        group_t _group;
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
};