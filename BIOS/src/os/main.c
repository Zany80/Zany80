#include <output.h>
#include <system.h>
#include <stdbool.h>
#include <os/file_system.h>

extern unsigned char version[4];
char version_str[5];

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
	text("ZanyOS v", 0, 1);
	text(version_str, (8 * GLYPH_WIDTH << 8) | 0, 1);
	text("Booting...", 0 | GLYPH_HEIGHT, 1);
}

void invalidImage() {
	cls(4);
	text("Disk not recognized!", 0, 1);
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

void disk() {
	node_t *file_system = (node_t*)0x4004;
	node_t *child1 = getNode(file_system, "main.zad");
	cls(4);
	if (child1 != 0) {
		if (child1->is_file) {
			if (child1->file->executable) {
				((function)child1->file->data)();
			}
			else {
				text(child1->file->data, 0 | GLYPH_HEIGHT, 1);
			}
		}
		else {
			text("main.zad shouldn't be a folder!", GLYPH_HEIGHT, 2);
		}
	}
	else
		text("Error!", GLYPH_HEIGHT, 2);
	text("Disk inserted!", 0, 1);
}

void noDisk() {
	cls(4);
	text("No disk inserted!", 0, 1);
}

char getDiskPresent() __naked {
	__asm
	in a, (1)
	ld l, a
	ret
	__endasm;
}

void main_disk() {
	mapInFileSystem();
	setRenderFunction(disk);
	while (true) {
		halt();
	}
}

void main_nodisk() {
	setRenderFunction(noDisk);
	while (true) {
		halt();
		if (getDiskPresent() > 0) {
			jumpWithoutStack(main_disk);
		}
	}
}

void main() {
	// convert the version number to a string, storing it in the pre-allocated
	// area in the data bank
	unsigned char i;
	for (i = 0; i < 4; i++) {
		version_str[i] = version[i] + '0';
	}
	version_str[4] = i = 0;
	setRenderFunction(startupScreen);
	while (true) {
		halt();
		if (i++ > 3 seconds) {
			if (getDiskPresent() > 0) {
				jumpWithoutStack(main_disk);
			}
			else {
				jumpWithoutStack(main_nodisk);
			}
		}
	}
}
