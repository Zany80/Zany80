0.1.0: (first entry, is a comparison to the prior commit - 424ac5a7f3d1f353609e54880d9bc0bf12d6bb78)
	Command scrolling - if a command exceeds the width of the screen it will auto scroll
	Assembler and compiler invocation from the simple_shell plugin is improved
		Can assemble a simple program with `assemble file.asm -o file.o` and `link file.o -o file.rom -e`
		`-e`/`--embed` and `-c`/`--link-crt` options added to the `link` command. `-e` and `--embed` are equivalent, as are `-c` and `--link-crt`.
		`-e` embeds the specified object file within a template file system, using the `_rom_name` symbol as the contents of the `/metadata/rom_name` file,
		and `_main` as the main function / contents of `/main.zad`.
		`-c` links in the C run time, which is needed when compiling C.
		`compile a.c -o a.o` and `link a.o -o a.rom -e -c` functions for compiling C.
	Minor optimizations to the shell's main loop, slightly lowering CPU usage at the expense of a tiny (<2MB *at most*, probably not even 1 MB) amount of RAM.
	Added `getKeys` wrapper for C.
	`true` and `false` are now usable in C (stdbool.h header added and automatically included in every C file).
