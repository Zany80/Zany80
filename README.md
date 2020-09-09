# Zany80

Zany80 is an IDE to make working with various pieces of hardware easier, tightly
integrating various emulators with debuggers and an intelligent code editor. It
also provides tools to ease developments of various types of software (port
monitors, sprite editors, etc).

As of version 0.2, Zany80 also includes support for a second system based on the
[LIMN architecture](https://github.com/limnarch), and a fully function toolchain
for the associated DragonFruit language.

There was once a webapp, compiled to emscripten. I am unwilling to deal
with the maintainance burden of supporting webapp bullshit, so unless
someone else does it, it will not happen.

Licensing note: some utility files have been borrowed from various open-source
projects. Their code is organized by source in lib/, and all have been
included in accordance with their respective licenses. A single LICENSE file
should be available in each folder (if it's missing, please file a ticket).

## Why use XML as a data format?

Honestly, XML was chosen mostly because I hadn't had any experience with it when
I made the decision. At this point, I don't really care enough to change it.
Patches are welcome, so long as performance is within the same order of
magnitude and the new format (*and its implementation*) are *simpler* than the
current one (not that that's hard). Also, it must be in C99, with no
dependency on C++ (or any other language, for that matter).

