* That's an use case for z80cpp library. Compile with:

g++ -Wall -O3 -std=c++14 -I../include z80sim.cpp -o z80sim -L../build -lz80cpp -Wl,-rpath=../build/

after build the library (of course).

