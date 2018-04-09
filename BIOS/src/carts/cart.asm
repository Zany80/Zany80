;--------------------------------------------------------
; File Created by SDCC : free open source ANSI-C Compiler
; Version 3.4.1 #9072 (Apr  3 2018) (Linux)
; This file was generated Sat Apr  7 03:48:29 2018
;--------------------------------------------------------
	.module hello_world
	.optsdcc -mz80
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _rom_name
	.globl _text
	.globl __text
	.globl _cls
	.globl _halt
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _RSEG (ABS)
	.org 0x4000
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _CODE
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _CODE
	.area _CODE
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
	.map	hello_world.c, 12, "void halt() {"
;	---------------------------------
; Function halt
; ---------------------------------
_halt_start::
_halt:
	.map	hello_world.c, 13, "__asm__("halt");"
	halt
	ret
	_halt_end::
.function	_halt, _halt_start, _halt_end
	.map	hello_world.c, 16, "void cls(char color) {"
;	---------------------------------
; Function cls
; ---------------------------------
_cls_start::
_cls:
	.map	hello_world.c, 23, "__endasm;"
	pop af \ pop bc
	push bc \ push af
	ld b, c
	ld a, 0
	call 0x8000
	.map	hello_world.c, 24, "color;"
	ret
	_cls_end::
.function	_cls, _cls_start, _cls_end
	.map	hello_world.c, 27, "void _text(const char *string, int position, char color) {"
;	---------------------------------
; Function _text
; ---------------------------------
__text_start::
__text:
	.map	hello_world.c, 36, "__endasm;"
	pop af
	pop hl \ pop bc \ pop de
	push de \ push bc \ push hl
	push af
	ld a, 1
	call 0x8000
	ret
	__text_end::
.function	__text, __text_start, __text_end
	.map	hello_world.c, 39, "void text(const char *string, char x, char y, char color) {"
;	---------------------------------
; Function text
; ---------------------------------
_text_start::
_text:
	push	ix
	ld	ix,#0
	add	ix,sp
	.map	hello_world.c, 40, "_text(string, (x << 8) | y, color);"
	ld	l,(ix + 6)
	ld	a,(ix + 6)
	rla
	sbc	a, a
	ld	h,a
	ld	h,#0x00
	ld	e,(ix + 7)
	ld	a,(ix + 7)
	rla
	sbc	a, a
	ld	d,a
	ld	a,e
	or	a, h
	ld	e,a
	ld	a,d
	or	a, l
	ld	d,a
	ld	a,(ix + 8)
	push	af
	inc	sp
	push	de
	ld	l,(ix + 4)
	ld	h,(ix + 5)
	push	hl
	call	__text
	pop	af
	pop	af
	inc	sp
	pop	ix
	ret
	_text_end::
.function	_text, _text_start, _text_end
	.map	hello_world.c, 43, "TITLE("Hello World!")"
;	---------------------------------
; Function rom_name
; ---------------------------------
_rom_name_start::
_rom_name:
	.asciiz "Hello World!" 
	_rom_name_end::
.function	_rom_name, _rom_name_start, _rom_name_end
	.map	hello_world.c, 45, "void main() {"
;	---------------------------------
; Function main
; ---------------------------------
_main_start::
_main:
	.map	hello_world.c, 47, "for (i = 0; i < 3 seconds; i++) {"
	ld	de,#0x0000
00102$:
	.map	hello_world.c, 48, "halt();"
	push	de
	call	_halt
	pop	de
	.map	hello_world.c, 49, "cls(0);"
	push	de
	xor	a, a
	push	af
	inc	sp
	call	_cls
	inc	sp
	pop	de
	.map	hello_world.c, 50, "text("Hello World", 0, 0, 3);"
	ld	bc,#___str_0
	push	de
	ld	hl,#0x0300
	push	hl
	xor	a, a
	push	af
	inc	sp
	push	bc
	call	_text
	pop	af
	pop	af
	inc	sp
	pop	de
	.map	hello_world.c, 47, "for (i = 0; i < 3 seconds; i++) {"
	inc	de
	ld	a,e
	sub	a, #0xB4
	ld	a,d
	rla
	ccf
	rra
	sbc	a, #0x80
	jp	C,00102$
	ret
	_main_end::
.function	_main, _main_start, _main_end
___str_0:
	.ascii "Hello World"
	.db 0x00
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
