#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "graphics.h"
#include "ring_buffer.h"

#include "serial.h"

static window_t *window;
static menu_t *menu;
static widget_t *output, *input;

static ring_buffer_t *input_buf;
static char output_buf[SERIAL_BUF_SIZE];
static size_t out_size;

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

void serial_init() {
	window = window_create("Serial Monitor");
	window_set_pos(window, 0, 20);
	window_min_size(window, 200, 80);
	window_register(window);
	menu = menu_create("Control");
	menu_append(menu, menuitem_create("Clear", serial_clear_output));
	window_append_menu(window, menu);
	output = label_set_wrapped(label_create(NULL), true);
	input = input_create(NULL, 128, input_handler);
	input->width = -2;
	window_append(window, output);
	window_append(window, input);
	out_size = 0;
	input_buf = ring_buffer_new(1024);
	if (!input_buf) {
    		// TODO: OOM solution
    		puts("OOM");
		exit(1);
	}
}

void serial_deinit() {
	window_destroy(window);
	menu_destroy_all(menu);
	menu_destroy(menu);
	widget_destroy(output);
	widget_destroy(input);
	ring_buffer_free(input_buf);
}
