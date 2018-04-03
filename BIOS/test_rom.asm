;; This is a test image
;;
;; Its filesystem is meant to be mapped to 0x4000 *only*. It's a test of the
;; BIOS.

.org 0x4000
.module TestROM_FS

metadata:
	.ascii "ZANY"

root_directory: ; node_t
	.dw name@root_directory ; char *name
	.db 0 ; bool is_file
	.dw dir@root_directory ; dir→ node_t **children
	.dw 0

.dir:
	.dw node0
	.dw test_file
	.dw main_zad
	.dw 0

.name:
	.asciiz "Test ROM"

node0:	; node_t
	.dw name@node0
	.db 1
	.dw file@node0

.name:
	.asciiz "File 1"
.file:
	.dw data@node0
	.dw 11
	.db 0
.data:
	.db "Hellothere", 0

test_file:	;node_t
	.dw name@test_file
	.db 1
	.dw file@test_file

.name:
	.asciiz "test_file"
.file:
	.dw data@test_file
	.dw 10
	.db 0
.data:
	.ascii "It works!"

main_zad:
	.dw name@main_zad
	.db 1
	.dw file@main_zad
.name:
	.asciiz "main.zad"
.file:
	.dw data@main_zad
	.dw end@main_zad - data@main_zad
	.db 1
.data:
	exx
	ld a, 0
	ld b, 3
	call 0x8000
	exx
	ret
.end:
