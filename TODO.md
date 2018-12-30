# Todo

* Re-do logging so that each module has its own individual log
* Split the BIOS build out of the Zany80 Travis configuration (which will help
with performance as KCC and Scas won't have to be installed for the builds)
* Add version info function to Plugin
* Multiple file support in editor (multiple buffers at once, multi-file projects)
* C support
* Built in help (tutorial plugin?)
* More examples for editor
* Finish implementing SimpleShell commands
* KnightOS support
* Merge legacy and oryolized plugins into one solid codebase
* Support alternative backends in addition to Oryol (Sokol, SFML, etc)
* ~~Strip out the experimental PPTR system, use normal pointers~~
* Remove dependency on C++
	* Rework plugin system to not use classes
* Standard plugin configurations
	* Standard
	* KnightOS
* Immediate input instead of text box
* Finish implementing breakpoints & debugging
* Unit tests
* Go over all the rushed parts and add cleanup (e.g. plugin removal)
* Ensure all error reporting goes through reportError
* Add other CPUs (e.g. 6809)
* Sound support
