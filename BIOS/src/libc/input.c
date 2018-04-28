char getKeys() __naked {
	__asm
	ld a, 2
	call 0x8000
	ld l, a
	ret
	__endasm;
}
