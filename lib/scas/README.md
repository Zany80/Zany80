# scas

[![builds.sr.ht status](https://builds.sr.ht/~maxleiter/scas/commits.svg)](https://builds.sr.ht/~maxleiter/scas/commits?)


Assembler and linker for z80.

## Status

We're finally just about done! All that's needed is a bit more testing before
1.0 proper can be released.

## Compiling from Source

Compiling under UNIX and Cygwin environments:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    sudo make install

Compiling on windows is recommended with [MSYS2](https://msys2.github.io/),
but will probably work with MinGW as well:

    mkdir build
    cd build
    cmake -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Release ..
    make
    make install

Don't forget to run the MSYS terminal as admin, or install under 
MSYS2 binaries with `-DCMAKE_INSTALL_PREFIX=/mingw64`.

Now read `man scas` to learn how to use it.

## Help, Bugs, Feedback

If you need help with KnightOS, want to keep up with progress, chat with
developers, or ask any other questions about KnightOS, you can hang out in the
IRC channel: [#knightos on irc.freenode.net](http://webchat.freenode.net/?channels=knightos).
 
To report bugs, please create [a GitHub issue](https://github.com/KnightOS/KnightOS/issues/new) or contact us on IRC.
 
If you'd like to contribute to the project, please see the [contribution guidelines](http://www.knightos.org/contributing).
