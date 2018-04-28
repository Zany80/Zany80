void halt() {
	__asm__("halt");
}

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