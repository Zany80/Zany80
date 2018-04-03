.globl _version

start:
	im 0
	ei
	jp _main
_version:
	.include <version_info>

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

call_renderer:
	push hl
	ld hl, (0x100)
	call __sdcc_call_hl
.return:
	pop hl
	ret

.global __sdcc_call_hl
__sdcc_call_hl:
	jp (hl)

.block 0x102 - $
