start:
	im 0
	ei
	jp main
version:
	.db 0,0,0,1

.block 0x10 - $

vsync_handler:
	ei
	reti

.block 0x18 - $

key_handler:
	ld a, 1
	ei
	reti

.block 0x66 - $

nmi_handler:
	retn

.block 0x100 - $
main:
	ld b, 4
	ld ix, version
	ld hl, 0x4009
	call inttostr
	halt
	ld a, 0
	ld b, 0
	call 0x8000
	ld a, 1
	ld hl, 0x4000
	ld bc, 0x0000
	ld e, 1
	call 0x8000
	ld a, 1
	ld hl, 0x4007
	ld bc, 0x2A00
	ld e, 1
	call 0x8000
	ld a, 1
	ld hl, 0x4009
	ld bc, 0x3000
	ld e, 1
	call 0x8000
	ld a, 1
	ld hl, 0x400E
	ld bc, 0x000A
	ld e, 1
	call 0x8000
; delay 90 frames to ensure prior screen is visible
	ld b, 90
.delay:
	halt
	djnz .delay
	in a, (1)
	cp 0
	jr z, no_cart

cart_detected:
.l:
	jr .l

no_cart:
	ld a, 0
	ld b, 2
	call 0x8000
	ld a, 1
	ld bc, 0x0000
	ld e, 1
	ld hl, 0x4019
	call 0x8000
.loop:
	halt
	in a, (1)
	cp 0
	jr z,.loop
	jr cart_detected

;inttostr
;B: digit count
;IX: int addr
;HL: string output
inttostr:
	push af
.loop:
	ld a, (ix)
	add a, 0x30
	ld (hl), a
	inc ix
	inc hl
	djnz .loop
	ld (hl), 0
	pop af
	ret
