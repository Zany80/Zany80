# Todo

## Short-term

* Code cleanup
* Rewrite frontend in C99 or Zig
* Replace Oryol with Sokol as primary frontend, remove C++ dependency entirely
* Fix all memory leaks
* Remove in-tree dependencies; require system builds

## Slightly less short-term

* Documentation. Lots of it.
* C support
* Multiple file support in editor (multiple buffers at once, multi-file projects)
* Built-in help (tutorial plugin?)
* More examples for editor
* Finish implementing breakpoints & debugging
* Video output, modeled after TI LCD
* Add z80e core

# Long term

* Support alternative backends in addition to Sokol (SFML, SDL, etc)
* Add other CPUs (e.g. 6809, AVR, 68k)
* Sound support
* KnightOS support
* Integrate a tricarbon JIT

