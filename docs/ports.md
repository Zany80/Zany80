# Ports

## Port 1:

### Disk manager commands
	0x00 000000aa:	Map disk. a is the bank to map, b is the disk to map there.
	0x00 0x04:		Map in the BIOS data bank. Meant for use by BIOS only.

### Power management commands

	0x01 0x00:		Shutdown
	0x01 0x01		Reboot
