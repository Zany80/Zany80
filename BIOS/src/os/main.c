#include <output.h>
#include <system.h>
#include <stdbool.h>
#include <os/file_system.h>

extern unsigned char version[4];
char version_str[5] = {0, 0, 0, 0, 0};

void setRenderFunction(function new_render) {
	__asm
	pop bc \ pop hl \ push hl \ push bc
	ld (0x100), hl
	__endasm;
	new_render;
}

function getRenderFunction() {
	__asm__("ld hl, (0x100)");
}

void startupScreen() {
	cls(4);
	text("ZanyOS v", 0, 0, 1);
	text(version_str, 8 * GLYPH_WIDTH, 0, 1);
	text("Booting...", 0, GLYPH_HEIGHT, 1);
}

void invalidImage() {
	cls(4);
	text("Disk not recognized!", 0, 0, 1);
}

bool strnequal(const char *s1, const char *s2, int length) {
	int index;
	for (index = 0; index < length; index++) {
		if (s1[index] != s2[index]) {
			return false;
		}
	}
	return true;
}

void mapInFileSystem() {
	__asm
	ld a, 0
	out (1), a
	inc a
	out (1), a
	__endasm;
	if (!strnequal((char*)0x4000, "ZANY", 4)) {
		setRenderFunction(invalidImage);
		while (true) {
			halt();
		}
	}
}

const node_t *main_zad;
const char *name;
const char *const default_name = "Disk name not specified";

void disk() {
	cls(4);
	text("Disk inserted: ", 0, 0, 1);
	text(name, 14 * GLYPH_WIDTH, 0, 1);
	if (main_zad) {
		if (main_zad->is_file) {
			if (main_zad->file->executable) {
				text("Do you wish to launch the application?", 0, GLYPH_HEIGHT, 1);
				text("\x01 to launch, \x02 to power off.", 0, GLYPH_HEIGHT * 2, 1);
			}
			else {
				text("Do you wish to view the data?", 0, GLYPH_HEIGHT, 1);
				text("\x01 to view, \x02 to power off.", 0, GLYPH_HEIGHT * 2, 1);
			}
		}
	}
	else {
		text("No main.zad file found!", 0, GLYPH_HEIGHT, 1);
	}
}

void noDisk() {
	cls(4);
	text("No disk inserted!", 0, 0, 1);
	text("Insert a disk, or press \x01 to shut down.", 0, GLYPH_HEIGHT, 1);
}

char getDiskPresent() __naked {
	__asm
	in a, (1)
	ld l, a
	ret
	__endasm;
}

void post_execute() {
	cls(4);
	text("Application finished! Press any button to power off.", 0, 0, 1);
}

bool released = false;

void render_file() {
	cls(4);
	text("Press any button to power off.", 0, 0, 1);
	text(main_zad->file->data, 0, GLYPH_HEIGHT * 2, 1);
}

void show_file() {
	setRenderFunction(render_file);
	while (true) {
		halt();
		if (released && getKeys())
			shutdown();
		if (!released && !getKeys())
			released = true;
	}
}

void null_function(){}

void execute_application() {
	setRenderFunction(null_function);
	((function)main_zad->file->data)();
	// after application exits, set render function to post_execute
	setRenderFunction(post_execute);
	while(true) {
		halt();
		if (released && getKeys())
			shutdown();
		if (!released && !getKeys())
			released = true;
	}
}

void main_disk() {
	mapInFileSystem();
	main_zad = getNode(&((zanyfs_t*)0x4000)->root, "main.zad");
	name = default_name;
	{
		node_t *metadata = getNode(&((zanyfs_t*)0x4000)->root, "metadata");
		if (metadata && !metadata->is_file) {
			metadata = getNode(metadata, "rom_name");
			if (metadata && metadata->is_file) {
				name = metadata->file->data;
			}
		}
	}
	setRenderFunction(disk);
	while (true) {
		halt();
		switch (getKeys()) {
			case 1:
				if (main_zad->file->executable)
					jumpWithoutStack(execute_application);
				else
					jumpWithoutStack(show_file);
				break;
			case 2:
				shutdown();
				break;
		}
	}
}

void main_nodisk() {
	setRenderFunction(noDisk);
	while (true) {
		halt();
		// Do this only once per frame (ish - the halt will sync to
		// vsync for the most part), but *outside* the interrupt so that
		// the destruction of the stack doesn't cause a crash.
		if (getDiskPresent() > 0) {
			jumpWithoutStack(main_disk);
		}
		if (getKeys() == 1)
			shutdown();
	}
}

void main() {
	// convert the version number to a string
	unsigned char i;
	for (i = 0; i < 4; i++) {
		version_str[i] = version[i] + '0';
	}
	i = 0;
	setRenderFunction(startupScreen);
	while (true) {
		halt();
		if (i++ > 1 seconds) {
			if (getDiskPresent() > 0) {
				jumpWithoutStack(main_disk);
			}
			else {
				jumpWithoutStack(main_nodisk);
			}
		}
	}
}

void shutdown() {
	__asm
	ld a, 1
	out (1), a
	ld a, 0
	out (1), a
	__endasm;
}

void reset() {
	__asm
	ld a, 0
	out (1), a
	ld a, 4
	out (1), a
	__endasm;
	setRenderFunction(startupScreen);	
	jumpWithoutStack(main);
}
