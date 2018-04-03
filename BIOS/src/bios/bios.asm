.globl function_table

bios_call:
	push hl
	push de
	ld h, 0
	ld l, a
	add hl, hl
	ex hl, de
	ld hl, function_table
	add hl, de
	ld e, (hl)
	inc hl
	ld d, (hl)
	ex hl, de
	pop de
	call call_hl
	inc sp \ inc sp
	ret

call_hl:
	push hl
	inc sp \ inc sp \ inc sp \ inc sp
	pop hl \ push hl
	dec sp \ dec sp \ dec sp \ dec sp
	ret
