start:
	im 0
	ei
	jp main
version:
	.db 0,0,0,1

.block 0x10 - $

vsync_handler:
	exx
	call call_renderer
	exx
	ei
	reti

.block 0x66 - $

nmi_handler:
	retn

.block 0x100 - $
.dw rmain
; delay 90 frames to ensure prior screen is visible
main:
	ld hl, rmain
	ld (0x100), hl
	ld b, 4
	ld ix, version
	ld hl, (0x4002)
	call inttostr
	halt
	call rmain
	ld b, 90
.delay:
	halt
	djnz .delay
	in a, (1)
	cp 0
	jp z, no_cart
	jp cart_detected

rmain:
	ld a, 0
	ld b, 0
	call 0x8000
	ld a, 1
	ld hl, (0x4000)
	ld bc, 0x0000
	ld e, 1
	call 0x8000
	ld a, 1
	ld hl, (0x4002)
	ld bc, 0x3000
	ld e, 1
	call 0x8000
	ld a, 1
	ld hl, (0x4004)
	ld bc, 0x000A
	ld e, 1
	call 0x8000
	ret

selected_disk: .db 0

cart_detected:
	ld hl, .render
	ld (0x100), hl
	in a, (1)
	call limit_cart
	; map the disk into 0x4000
	ld hl, selected_disk
	ld (hl), a
	call map_disk
	ld hl, 0x4000
	call verify_disk
	jp z, invalid_cart
	; read the title
	ld de, .title
	ld bc, 0x4004
	ld a, 3
	call 0x8000
	call restore_bios_data
	jr $
.title: .block 32
.render:
	ld a, 0
	ld b, 0
	call 0x8000
	ld a, 1
	ld hl, (0x4008)
	ld e, 1
	ld bc, 0x0000
	call 0x8000
	ld hl, .title
	ld a, 1
	ld e, 4
	ld bc, 0x4010
	call 0x8000
	ld hl, (0x400A)
	ld e, 1
	ld bc, 0x0010
	ld a, 1
	call 0x8000
	ld a, 1
	ld hl, (0x400C)
	ld e, 1
	ld bc, 0x0020
	call 0x8000
	ret

no_cart:
	ld hl, .render
	ld (0x100), hl
.loop:
	ld b, 255
.delay:
	halt
	djnz .delay
	in a, (1)
	cp 0
	jp z,.loop
	jp cart_detected
.render:
	ld a, 0
	ld b, 2
	call 0x8000
	ld a, 1
	ld bc, 0x0000
	ld e, 1
	ld hl, (0x4006)
	call 0x8000
	ret

invalid_cart:
	call restore_bios_data
	ld hl, .render
	ld (0x100), hl
.l:
	halt
	jr .l
.render:
	ld a, 0
	ld b, 2
	call 0x8000
	ld a, 1
	ld bc, 0x0000
	ld e, 1
	ld hl, (0x400E)
	call 0x8000
	ret

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

call_renderer:
	push hl
	ld hl, (0x100)
	call .call_hl
.return:
	pop hl
	ret
	
.call_hl:
	jp (hl)

;; limit_cart:
;; Set a to either 0 or 1. If cart 0 is present, will return 0. Else, 1.
;; If no carts are present, UNDEFINED BEHAVIOR! In practice, will still return 0
;; but such behavior MUST NOT BE DEPENDED UPON!
limit_cart:
	bit 1, a
	jp z, .zero
	bit 2, a
	jp z, .one
.zero:
	ld a, 0
	ret
.one:
	ld a, 1
	ret

map_disk:
	ld b, a
	ld a, 0
	out (1), a
	ld a, b
	set 2, a
	out (1), a
	ret

; verifies disk image.
; inputs:
;	hl: address of disk
; outputs:
;	b: 0 if invalid, 1 if valid
verify_disk:
	call .verify
	ld a, b
	cp 0
	ret
.verify:
	ld b, 0
	ld a, (hl)
	cp 'Z'
	ret nz
	inc hl
	ld a, (hl)
	cp 'A'
	ret nz
	inc hl
	ld a, (hl)
	cp 'N'
	ret nz
	inc hl
	ld a, (hl)
	cp 'Y'
	ret nz
	ld b, 1
	ret

restore_bios_data:
	ld a, 0
	out (1), a
	ld a, 8
	out (1), a
	ret
