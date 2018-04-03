#include <system.h>

void jump(function f) __naked {
	f;
	__asm
	// discard return address
	pop hl
	// get target address
	pop hl
	// jump!
	jp (hl)
	__endasm;
}

void jumpWithoutStack(function f) __naked {
	f;
	__asm
	// discard return address
	pop hl
	// get target address
	pop hl
	// reset stack
	ld sp, 0
	// jump!
	jp (hl)
	__endasm;
}
