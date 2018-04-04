.globl cls
cls:
	ld a, 1
	out (0), a
	ret

.globl text
text:
	ld a, 0
	out (0), a
	ret

.globl get_keys
get_keys:
	in a, (0)
	ret

.globl zanyfs_gettitle
.globl _zanyfs_getname
zanyfs_gettitle:
	; de contains address to store string in
	; bc contains address to read from
	push de
	push bc
	call _getName
	pop bc
	pop de
	ret

.globl zanyfs_getnode
zanyfs_getnode:
	 ;hl contains name of node to find
	 ;bc contains parent node
	push hl
	push bc
	call _getNode
	pop bc
	; slightly faster to pop than to `inc sp` twice
	pop de
	ret
