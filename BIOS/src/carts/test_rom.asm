;; Hello, world

.org 0x4000
.module HelloWorld

fs:			; zanyfs_t
zany:		; char[4]
	.ascii "ZANY"
root:		; node_t
	.dw name@root ; char *name
	.db 0 ; bool is_file
	.dw dir@root ; dir→ node_t **children
	.dw 0 ; node_t *parent
	.dw init_heap_begin
	.dw init_heap_end
	.dw init_heap_begin

.name:
	.asciiz "/"
.dir:
	;node_t **children
	.dw main_zad
	.dw metadata
	.dw 0

main_zad:
	.dw name@main_zad
	.db 1
	.dw file@main_zad
	.dw root

.name:
	.asciiz "main.zad"

.file:
	.dw data@main_zad
	.dw 1
	.db 1
.data:
	jp .wait_no_key
.render:
	ld a, 0
	call 0x8000
	ld a, 1
	ld hl, .looping
	ld bc, 0
	ld e, 1
	call 0x8000
	halt
	ret
.wait_no_key:
	ld b, 6
	call .render
	ld a, 2
	call 0x8000
	cp 0
	jp nz, .wait_no_key
.wait_key:
	ld b, 4
	call .render
	ld a, 2
	call 0x8000
	cp 0
	jp z, .wait_key
	ret
.looping: .asciiz "Hello, World! Press any key to exit."

metadata:
	.dw name@metadata
	.db 0
	.dw dir@metadata
	.dw root

.name: .asciiz "metadata"
.dir:
	.dw file_name
	.dw 0

file_name:
	.dw name@file_name
	.db 1
	.dw file@file_name
	.dw metadata

.name: .asciiz "rom_name"
.file:
	.dw data@file_name
	.dw 10
	.db 0
.data:
	.asciiz "Hello, world"

; heap: the following area is used to allocate new files/folders
init_heap_begin:

; first header: initially has next as the end of the heap and next_free as null
; after it is written to, this will change

.dw init_heap_end	; header_t *next
.dw 0				; header_t *next_free - no other free block


.block 0x8000 - $
init_heap_end:
