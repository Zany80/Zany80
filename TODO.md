# Todo

## Short-term

* Unit tests
* Go over all the rushed parts and fix them (e.g. plugin removal)
* Ensure all error reporting goes through report_error
* Re-do logging so that each module has its own individual log
* Add version info to plugin interface
* Documentation. Lots of it.
* Re-do imGuiTextEditor fips fork to use a subproject for ease of upgrading
* Non-debug display (32x20 character LCD)
* PortableApps Windows version

## Slightly less short-term

* Lua plugin wrapper
* C support
	* Lua support via eLua. Will need to add KCC support to eLua, then z80 support.
* Multiple file support in editor (multiple buffers at once, multi-file projects)
* Built in help (tutorial plugin?)
* More examples for editor
* Finish implementing breakpoints & debugging
* Fix all memory leaks
* Proper dependency cleanups on unloads
	* (If a tries b which loads c, but b then fails for a, c needs to be unloaded also)
	* This requires dependency storage
* GPU plugin
	* ~~Expose low-level graphics API (from Oryol) to CPU~~
		* Revisit in the future, not worth it right now
	* Simple pixel framebuffer
* Add z80e core
* Use backtrace/stackwalker to find module names in logging functions

# Long term

* Replace Oryol with Sokol as primary backend, remove C++ dependency entirely
	* Support alternative backends in addition to Sokol (SFML, SDL, etc)
* Add other CPUs (e.g. 6809, AVR)
* LIMNVM support?
* Sound support
* KnightOS support
* Write a dynamic recompiler for x86_64

# Completed

* ~~Rewrite scas plugin in pure C~~
* ~~Make error box in text editor resizable~~
* ~~Finish migrating plugins to new API~~
* ~~Fully restore text-based functionality using built-in editor and assembly~~
* ~~Split the BIOS build out of the Zany80 Travis configuration (which will help
with performance as KCC and Scas won't have to be installed for the builds)~~
* ~~Finish implementing SimpleShell commands~~ **Cancelled, shell removed**
* ~~Immediate input instead of text box~~
* ~~Merge legacy and oryolized plugins into one solid codebase~~
	* Removed legacy code instead, supporting multiple ABIs is almost never worth it
* ~~Strip out the experimental PPTR system, use normal pointers~~
* ~~Remove dependency on C++~~
	* ~~Rework plugin system to not use classes~~
		* Plugins can now be in standard C, no C++ required whatsoever
