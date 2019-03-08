# Todo

* Re-do logging so that each module has its own individual log
* Split the BIOS build out of the Zany80 Travis configuration (which will help
with performance as KCC and Scas won't have to be installed for the builds)
* Add version info function to Plugin
* Multiple file support in editor (multiple buffers at once, multi-file projects)
* Finish migrating plugins to new API
* Built in help (tutorial plugin?)
* More examples for editor
* Finish implementing SimpleShell commands
* KnightOS support
* ~~Merge legacy and oryolized plugins into one solid codebase~~
	* Removed legacy code instead, supporting multiple ABIs is almost never worth it
* Support alternative backends in addition to Oryol (Sokol, SFML, etc)
* ~~Strip out the experimental PPTR system, use normal pointers~~
* ~~Remove dependency on C++~~
	* ~~Rework plugin system to not use classes~~
* Standard plugin configurations
	* Standard
	* KnightOS
* Immediate input instead of text box
* Finish implementing breakpoints & debugging
* Unit tests
* Go over all the rushed parts and add cleanup (e.g. plugin removal)
* Ensure all error reporting goes through report_error
* Add other CPUs (e.g. 6809, AVR)
* Sound support
* Run under valgrind, ensure no memory leaks
* Proper dependency cleanups on unloads
	(If a tries b which loads c, but b then fails for a, c needs to be unloaded also)
* GPU plugin
	* ~~Expose low-level graphics API (from Oryol) to CPU~~
		* Revisit in the future, need to research security implications
	* Simple framebuffer
* Rewrite scas plugin in pure C
* Replace Oryol with Sokol, remove C++ dependency entirely
