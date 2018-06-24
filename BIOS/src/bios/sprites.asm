.globl sprite_upload
sprite_upload:
	ld a, 2
	out (0), a
	ret

.globl sprite_draw
sprite_draw:
	ld a, 3
	out (0), a
	ret
