.globl cls
.globl text
.globl get_keys
.globl zanyfs_gettitle
.globl zanyfs_getnode
.globl sprite_upload
.globl sprite_draw

.globl function_table

function_table:
	.dw cls
	.dw text
	.dw get_keys
	.dw zanyfs_gettitle
	.dw zanyfs_getnode
	.dw sprite_upload
	.dw sprite_draw
