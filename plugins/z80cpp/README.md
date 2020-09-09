# fips-z80cpp
## Z80 core in C++

This is a port from Java to C++ of JSanchez's [Z80Core](https://github.com/jsanchezv/Z80Core),
forked from his original repository to improve interrupt mode support and use
the fips build system.

## Usage

To use this, add it as an import to a fips project in fips.yml.

## Features

The core have the same features of [Z80Core](https://github.com/jsanchezv/Z80Core):

* Complete instruction set emulation
* Emulates the undocumented bits 3 & 5 from flags register
* Emulates the MEMPTR register (known as WZ in official Zilog documentation)
* Strict execution order for every instruction
* Precise timing for all instructions, totally decoupled from the core

*jspeccy at gmail dot com*
