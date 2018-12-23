#!/bin/bash
./fips fetch
sed -e 's/sdk-incoming-64bit/sdk-1.38.12-64bit/' build/fips/mod/emscripten.py -i
sed build/fips/cmake-toolchains/emscripten.toolchain.cmake -e 's/incoming/1.38.12/' -i
./fips setup emscripten
