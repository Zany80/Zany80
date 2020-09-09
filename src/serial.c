#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "graphics.h"
#include "ring_buffer.h"

#include "serial.h"

static window_t *window;
static widget_t *output, *input, *clear;
static menu_t *global_menu;

static ring_buffer_t *input_buf;
static char output_buf[SERIAL_BUF_SIZE];
static size_t out_size;
bool serial_is_docked;

uint32_t serial_read() {
	// TODO: determine attached caller (caller-id parameter?)
	return ring_buffer_read(input_buf);
}

void serial_write(uint32_t value) {
	if (value == 0) {
		return;
	}
	if (out_size >= 1024 * 32) {
		// TODO: integrate with permissions / UI feedback?
		serial_clear_output();
	}
	output_buf[out_size++] = (char)(value & 0xFF);
	output_buf[out_size] = 0;
	widget_set_label(output, output_buf);
}

void serial_clear_output() {
	out_size = 0;
	output_buf[0] = 0;
	widget_set_label(output, output_buf);
}

static void input_handler(widget_t *input) {
	char *msg = input_get_text(input);
	{
		char *m = malloc(strlen(msg) + 2);
		if (m == NULL) {
				// TODO: determine OOM solution
			puts("Failed to allocate memory");
			exit(1);
		}
		strcat(strcpy(m, msg), "\n");
		free(msg);
		msg = m;
	}
	for (size_t i = 0; i < strlen(msg); i++) {
		ring_buffer_append(input_buf, msg + i, 1);
		// TODO: add option to hook in
	}
	free(msg);
	input_set_text(input, "");
}

void serial_init(bool docked) {
	serial_is_docked = docked;
	window = window_create("Serial Monitor");
	window_set_pos(window, 100, 80);
	window_min_size(window, 200, 100);
	window_initial_size(window, 700, 300);
	clear = menuitem_create("Clear", serial_clear_output);
	global_menu = menu_create("Serial");
	menu_append(global_menu, clear);
	window_append(window, clear);
	output = label_set_wrapped(label_create(NULL), true);
	input = input_create(NULL, 128, input_handler);
	input->width = -2;
	window_append(window, input);
	window_append(window, output);
	out_size = 0;
	input_buf = ring_buffer_new(1024);
	if (!input_buf) {
		// TODO: OOM solution
		puts("OOM");
		exit(1);
	}
	if (docked) {
		window_t *root = get_root();
		window_append_menu(root, global_menu);
		window_append(root, input);
		window_append(root, output);
	}
	else {
		window_register(window);
	}
}

void serial_deinit() {
	window_unregister(window);
	window_destroy(window);
	widget_destroy(output);
	widget_destroy(input);
	menu_destroy(global_menu);
	widget_destroy(clear);
	ring_buffer_free(input_buf);
}

void serial_toggle_root() {
	window_t *root = get_root();
	// This is set to the *new* value.
	if (!serial_is_docked) {
		window_remove(root, clear);
		window_remove(root, input);
		window_remove(root, output);
		window_remove_menu(root, global_menu);
		window_register(window);
	}
	else {
		window_unregister(window);
		window_append_menu(root, global_menu);
		window_append(root, input);
		window_append(root, output);
	}
}

void serial_write_all(const char *const msg, int32_t len) {
	// TODO: optimize this
	if (len == -1) {
		len = strlen(msg);
	}
	for (int32_t i = 0; i < len; i += 1) {
		serial_write(msg[i]);
	}
}
