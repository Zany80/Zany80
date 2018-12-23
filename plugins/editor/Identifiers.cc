#include "Identifiers.h"

Map<const char *, const char *> Z80Instructions = {
	{ "ld", "LoaD a value into a register or memory" },
	{ "call", "Calls a function" },
	{ "jp", "Absolute jump to anywhere in the current address space" },
	{ "jr", "Relative jump\nA relative jump takes two bytes instead of three,\n"
			"but is slightly slower, and can only be used within a small\n"
			"distance from the target." },
};

Map<const char *, const char *> Zany80Functions = {
	{ "putchar","Takes a character from register a and\n"
				"writes it over the serial port." },
	{ "serial_write", 	"Takes the string pointed to by the hl register\n"
						"and writes it over the serial port."},
	{ "_waitchar", 	"Waits for input to arrive over the serial port.\n"
					"Returns the character in the l register.\n"
					"IMPORTANT: right now, the newline character will be received after each message.\n"
					"It can be discarded, or used to mark the end of a message.\n"
					"To discard, you can simply load the character into the a register and\n"
					"write `cp '\\n' \\ jr z, .discard`."}
};

Map<const char *, const char *> Registers = {
	{ "a", "The 8-bit accumulator register" },
	{ "hl", "The 16-bit HL register pair, an aggregate of the 8-bit H and L registers" }
};

Map<const char *, const char *> Directives = {
	{ "asciiz","Writes the following ASCII string directly into memory,\n"
				"automatically adding the terminating zero."},
	
};

Array<Map<const char *, const char *>> identifiers = {
	Z80Instructions, Zany80Functions, Registers, Directives
};
