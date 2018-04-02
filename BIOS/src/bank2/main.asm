.dw os1
.dw version
.dw booting
.dw nodisk
.dw disk
.dw name
.dw load
.dw invalid_cart
.dw file_exists
.dw file_not_exists

os1:
; 9 bytes
.asciiz "ZanyOS v"

version:
; store version string
.block 5

booting:
.asciiz "Booting..."

nodisk:
.asciiz "No disk inserted!"

disk:
.asciiz "Disk detected."

name:
.asciiz "ROM name: "

load:
.asciiz "Load ROM? A for yes, right arrow to examine other disk slot."

invalid_cart:
.asciiz "Disk not recognized."

file_exists:
.asciiz "File exists!"

file_not_exists:
.asciiz "File does not exist!"
