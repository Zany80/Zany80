void cls() __naked {
	__asm
	ld a, 1
	out (0), a
	ret
	__endasm;
}

void text() __naked {
	__asm
	inc sp \ inc sp
	pop hl \ push hl
	dec sp \ dec sp
	ld a, 0
	out (0), a
	ret
	__endasm;
}
