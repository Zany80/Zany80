#include <os/file_system.h>

void cls(char color) {
	__asm
	pop af \ pop bc
	push bc \ push af
	ld b, c
	ld a, 0
	call 0x8000
	__endasm;
	color;
}

void _text(const char *string, int position, char color) {
	string, position, color;
	__asm
	pop af
	pop hl \ pop bc \ pop de
	push de \ push bc \ push hl
	push af
	ld a, 1
	call 0x8000
	__endasm;
}

void text(const char *string, char x, char y, char color) {
	_text(string, (x << 8) | y, color);
}

node_t *getNode(node_t *parent, char *name) __naked {
	parent, name;
	__asm
	pop af \ pop bc \ pop hl
	push hl \ push bc \ push af
	ld a, 4
	call 0x8000
	ret
	__endasm;
}

unsigned char getKeys() __naked {
	__asm
	ld a, 2
	call 0x8000
	ld l, a
	ret
	__endasm;
}

//~ node_t *createFile(const zanyfs_t *const file_system, const char *const name, node_t *const parent) {
	//~ __asm
	
	//~ __endasm;
//~ }
