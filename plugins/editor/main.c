#define stdlib_start_text ".export main\n\
main:\n\
	ld a, 0\n\
	out (1), a\n\
	ld hl, msg\n\
	ld c, 1\n\
	call write\n\
	inc c\n\
	call write\n\
	; Switch to serial mode\n\
	ld a, 1\n\
	call set_keyboard\n\
.loop:\n\
	ld a, 2\n\
	call wait_interrupt\n\
	call get_key\n\
	cp -1\n\
	jr z, .loop\n\
	ld hl, .received\n\
	ld c, 2\n\
	call write\n\
	cp 26\n\
	jr c, .letter\n\
	add a, '0' - 26\n\
	jr .show\n\
.letter:\n\
	add a, 'a'\n\
.show:\n\
	out (1), a\n\
	out (2), a\n\
	ld a, '\\n'\n\
	out (2), a\n\
	jr .loop\n\
.received: .asciiz \"Character received: \"\n\
msg: .asciiz \"ROM built by Zany80 Scas plugin...\\n\""

#define lib_less "\
.equ mmu, 0\n\
\n\
start:\n\
\tjp boot\n\
\n\
.block 0x100 - $\n\
boot:\n\
\tld a, 1\n\
\tld b, 3\n\
.loop:\n\
\tout (mmu), a\n\
\tout (mmu), a\n\
\tinc a\n\
\tdjnz .loop\n\
\n\
\tld hl, 0\n\
\tld de, 0x4000\n\
\tld bc, 0x4000\n\
\tldir\n\
\n\
\tld a, 0\n\
\tout (mmu), a\n\
\tinc a\n\
\tout (mmu), a\n\
\tout (mmu), a\n\
\tld a, 4\n\
\tout (mmu), a\n\
\n\
\tld sp, 0xFF00\n\
\tld hl, .str\n\
\tld c, 1\n\
\tcall write\n\
\tinc c\n\
\tcall write\n\
\t\n\
.end:\n\
\tin a, (1)\n\
\tcp -1\n\
\tjr z, .end\n\
\tout (1), a\n\
\tld hl, .received\n\
\tld c, 2\n\
\tcall write\n\
\tout (2), a\n\
\tld a, '\\n'\n\
\tout (2), a\n\
\tjr .end\n\
\n\
.str: .asciiz \"Hello, world!\\n\"\n\
.received: .asciiz \"Received: \"\n\
\n\
; hl: string\n\
; c: output port\n\
write:\n\
\tpush af\n\
\tpush hl\n\
.loop:\n\
\tld a, (hl)\n\
\tcp 0\n\
\tjr z, .done\n\
\tout (c), a\n\
\tinc hl\n\
\tjr .loop\n\
.done:\n\
\tpop hl\n\
\tpop af\n\
\tret\n\
"
#include <Zany80/Plugin.h>
#include <Zany80/API.h>
#include <3rd-party/scas/stringop.h>
#include <editor_config.h>

#include <stdio.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

window_t *main_window, *message_log;
menu_t *file_menu, *tool_menu, *asm_select, *cpu_select;
widget_t *asms, *cpus;
widget_t *editor;
widget_t *label1, *error_label, *tool_info;
widget_t *info_entry, *tool_select;
bool stdlib, always_log;
char *path;
char *unmodified;
char *errors, *tool;

list_t *toolchain_list, *cpu_list;

plugin_t *cpu;
plugin_t *assembler;

plugin_t disabled = {
	.name = "None (Disabled)"
};

void new_file() {
	if (unmodified) {
		free(unmodified);
	}
	if (path) {
		free(path);
	}
	input_set_text(editor, stdlib ? stdlib_start_text: lib_less);
	unmodified = input_get_text(editor);
	path = NULL;
}

enum {
	open, save, none
} action_type;

void save_file() {
	if (!path) {
		action_type = save;
		widget_set_visible(info_entry, true);
	}
	else {
		FILE *f = fopen(path, "w");
		if (f) {
			free(unmodified);
			unmodified = input_get_text(editor);
			fwrite(unmodified, 1, strlen(unmodified), f);
			fflush(f);
			fclose(f);
		}
	}
}

void open_file() {
	action_type = open;
	widget_set_visible(info_entry, true);
}

bool supports(const char *type) {
	return (!strcmp(type, "Perpetual")) || (!strcmp(type, "Editor"));
}

int current_cpu, current_assembler;

void cpu_handler(int index) {
	if (index < cpu_list->length) {
		cpu = (plugin_t*)(cpu_list->items[index]);
	}
}

void asm_handler(int index) {
	if (index < toolchain_list->length) {
		assembler = (plugin_t*)(toolchain_list->items[index]);
	}
}

void gen_radio_list(widget_t *group, list_t *list, int *current, void(*handler)(int index)) {
	group_clear(group, true);
	for (int i = 0; i < list->length; i++) {
		plugin_t *plugin = (plugin_t*)(list->items[i]);
		group_add(group, radio_create(plugin->name, current, i, handler));
	}
}

void update_tool_info() {
	sprintf(tool, "%s%s ; %s%s", cpu ? "CPU: " : "CPU ", cpu ? cpu->name : "not connected", assembler ? "Assembler: " : "Assembler ", assembler ? assembler->name : "not connected");
	widget_set_label(tool_info, tool);
	list_t *current_cpus = get_plugins("CPU");
	list_t *current_asms = get_plugins("Toolchain");
	list_insert(current_cpus, 0, &disabled);
	list_insert(current_asms, 0, &disabled);
	if (cpu_list) {
		list_free(cpu_list);
	}
	cpu_list = current_cpus;
	gen_radio_list(cpus, cpu_list, &current_cpu, cpu_handler);
	if (toolchain_list) {
		list_free(toolchain_list);
	}
	toolchain_list = current_asms;
	gen_radio_list(asms, toolchain_list, &current_assembler, asm_handler);
}

void frame(float delta) {
	update_tool_info();
	char buf[128];
	char *current = input_get_text(editor);
	bool modified = strcmp(unmodified, current) != 0;
	free(current);
	sprintf(buf, "%s%s%s", modified ? "* " : "", path ? "File: " : "Untitled", path ? path : "");
	widget_set_label(label1, buf);
	render_window(main_window);
	if (!window_is_minimized(main_window)) {
		float x, y, width, height;
		window_get_pos(main_window, &x, &y);
		window_get_size(main_window, &width, &height);
		window_set_pos(message_log, x, y + height);
		window_min_size(message_log, width, 0);
		window_max_size(message_log, width, FLT_MAX);
		render_window(message_log);
	}
}

perpetual_plugin_t perpetual = {
	.frame = frame
};

plugin_t plugin = {
	.name = "Editor",
	.supports = supports,
	.perpetual = &perpetual
};

void hide_info_entry() {
	widget_set_visible(info_entry, false);
}

void handle_info(widget_t *input) {
	char *msg = input_get_text(input);
	switch (action_type) {
		case save:
			if (path) {
				free(path);
			}
			path = msg;
			hide_info_entry();
			save_file();
			break;
		case open:{
			FILE *f = fopen(msg, "r");
			if (f) {
				fseek(f, 0, SEEK_END);
				long size = ftell(f);
				rewind(f);
				char *buf = malloc(size + 1);
				fread(buf, 1, size, f);
				buf[size] = 0;
				fclose(f);
				path = msg;
				hide_info_entry();
				input_set_text(editor, buf);
				free(buf);
				free(unmodified);
				unmodified = input_get_text(editor);
			}
			}break;
		case none:
		default:
			break;
	}
}

void editor_log(char *message) {
	strcat(strcat(errors, message), "\n");
	widget_set_label(error_label, errors);
}

void clear_errors() {
	errors[0] = 0;
	editor_log("");
}

char *get_output_name() {
	char *output_name;
	if (path) {
		char *p = strdup(path);
		char *t = strtok(p, ".");
		output_name = malloc(strlen(t) + 6);
		strcpy(output_name, t);
		strcat(t, ".zany");
		free(p);
	}
	else {
		output_name = strdup("temp.rom");
	}
	return output_name;
}

void build() {
	if (path)
		save_file();
	clear_errors();
	if (assembler != &disabled) {
		list_t *conversions = assembler->toolchain->get_conversions();
		for (int i = 0; i < conversions->length; i++) {
			toolchain_conversion_t *conversion = conversions->items[i];
			if (!strcmp(conversion->source_ext, ".asm") && !strcmp(conversion->target_ext, ".o")) {
				// Attempt conversion
				list_t *sources = create_list();
				char *name;
				if (path) {
					list_add(sources, strdup(path));
				}
				else {
					// Dump to a temporary file
					name = tmpnam(NULL);
					path = name;
					save_file();
					path = NULL;
					list_add(sources, strdup(name));
				}
				if (stdlib) {
					char *root = zany_root_folder();
					char *buf = malloc(strlen(root) + 15);
					strcpy(buf, root);
					free(root);
					strcat(buf, "/lib/stdlib.o");
					list_add(sources, strdup(buf));
					free(buf);
				}
				char *out;
				char *output_name = get_output_name();
				int ret = assembler->toolchain->convert(sources, output_name, &out);
				if (ret == 0) {
					editor_log("Assembled successfully!\n");
				}
				if (ret != 0 || always_log) {
					editor_log(out);
				}
				free(out);
				if (!path) {
					remove(name);
				}
				free(output_name);
				free_flat_list(sources);
			}
		}
		list_free(conversions);
	}
	else {
		editor_log("No assembler selected! Please select an assembler in the Tools menu!");
	}
}

void execute() {
	clear_errors();
	if (cpu != &disabled) {
		if (cpu->cpu && cpu->cpu->load_rom) {
			char *output_name = get_output_name();
			if (cpu->cpu->load_rom(output_name)) {
				editor_log("ROM loaded.");
			}
			else {
				editor_log("Failed to load ROM!");
			}
			free(output_name);
		}
		else {
			editor_log("CPU plugin lacks external ROM loading functionality!");
		}
	}
	else {
		editor_log("No CPU selected! Please select a CPU in the Tools menu!");
	}
}

PLUGIN_EXPORT void init() {
	assembler = NULL;
	cpu = NULL;
	stdlib = false;
	unmodified = NULL;
	toolchain_list = cpu_list = NULL;
	tool = malloc(128);
	main_window = window_create("Editor##C");
	message_log = window_create("Editor##MessageWindow");
	window_initial_size(main_window, 600, 480);
	window_min_size(main_window, 400, 300);
	window_min_size(message_log, 400, 0);
	window_initial_size(message_log, 400, 78);
	window_set_titlebar(message_log, false);
	window_append_menu(main_window, file_menu = menu_create("File"));
	menu_append(file_menu, menuitem_create("New File", new_file));
	menu_append(file_menu, menuitem_create("Open File", open_file));
	menu_append(file_menu, menuitem_create("Save File", save_file));
	//~ menu_append(file_menu, checkbox_create("Use standard library", &stdlib, NULL));
	window_append_menu(main_window, tool_menu = menu_create("Tools"));
	menu_append(tool_menu, menuitem_create("Build", build));
	menu_append(tool_menu, menuitem_create("Execute", execute));
	asm_select = menu_create("Select Assembler");
	cpu_select = menu_create("Select CPU");
	asms = group_create();
	cpus = group_create();
	menu_append(tool_menu, submenu_create(asm_select));
	menu_append(tool_menu, submenu_create(cpu_select));
	menu_append(tool_menu, checkbox_create("Always show assembler logs", &always_log, NULL));
	menu_append(asm_select, asms);
	menu_append(cpu_select, cpus);
	info_entry = group_create();
	group_setorientation(info_entry, horizontal);
	group_add(info_entry, input_create("File name", 512, handle_info));
	group_add(info_entry, button_create("Cancel", hide_info_entry));
	hide_info_entry();
	window_append(main_window, info_entry);
	window_append(main_window, editor = editor_create("Text Editor"));
	window_append(message_log, label1 = label_create("Untitled"));
	window_append(message_log, tool_info = label_create("Tool info unknown"));
	new_file();
	errors = malloc(1024 * 1024);
	window_append(message_log, error_label = label_create(""));
	label_set_wrapped(error_label, true);
	update_tool_info();
	asm_handler(2);
	cpu_handler(1);
	current_assembler = 2;
	current_cpu = 1;
}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin;
}

PLUGIN_EXPORT void cleanup() {
	widget_destroy(error_label);
	free(errors);
	free(tool);
	group_clear(asms, true);
	group_clear(cpus, true);
	widget_destroy(asms);
	widget_destroy(cpus);
	menu_destroy_all(file_menu);
	menu_destroy(file_menu);
	menu_destroy_all(tool_menu);
	menu_destroy(tool_menu);
	group_clear(info_entry, true);
	widget_destroy(info_entry);
	widget_destroy(editor);
	widget_destroy(label1);
	widget_destroy(tool_info);
	window_destroy(main_window);
	window_destroy(message_log);
	free(unmodified);
	if (path) {
		free(path);
	}
}

#if ORYOL_EMSCRIPTEN
#include <emscripten.h>
EM_JS(void, download, (const char* name, const char* str), {
	var element = document.createElement('a');
	element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(UTF8ToString(str).split("\n").join("\\r\n")));
	element.setAttribute('download', UTF8ToString(name));
	element.style.display = 'none';
	document.body.appendChild(element);
	element.click();
	document.body.removeChild(element);
});

extern "C" void EMSCRIPTEN_KEEPALIVE EditorOpen(const char *name, const char *data) {
	if (path) {
		free(path);
	}
	window_remove(main_window, editor);
	window_append(main_window, editor = editor_create("Text Editor"));
	input_set_text(data);
	path = strdup(name);
}
#endif
