#define frames * 1
#define seconds * 60 frames
#define minutes * 60 seconds
#define hours * 60 minutes

#define TITLE(x) void rom_name() __naked { \
	__asm                                  \
	.asciiz x                              \
	__endasm;                              \
}

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

TITLE("Hello World!")

void main() {
	int i;
	for (i = 0; i < 3 seconds; i++) {
		halt();
		cls(0);
		text("Hello World", 0, 0, 3);
	}
}
