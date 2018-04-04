;; This is a test image
;;

.org 0x4000
.module TestROM_FS

fs:			; zanyfs_t
zany:
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
	.dw 0
	.db 1
.data:
	ret

; heap: the following area is used to allocate new files/folders
init_heap_begin:

; first header: initially has next as the end of the heap and next_free as null
; after it is written to, this will change

.dw init_heap_end	; header_t *next
.dw 0				; header_t *next_free - no other free block


.block 0x8000 - $
init_heap_end:
