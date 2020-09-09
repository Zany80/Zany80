SRC=$(wildcard *.c)
OBJ=$(addprefix bin/,$(addsuffix .o, $(basename $(SRC))))
ANTECEDENT_PATH=../a3x
ANTECEDENT_EXISTS=$(shell [ -e $(ANTECEDENT_PATH) ] && echo 1 || echo 0 )
DESTDIR=/usr/bin
CFLAGS=-O3 -g -pedantic -Werror -Wall -Wextra -I. -fPIC

bin/dragonfruit: $(OBJ)
	gcc $^ -o $@ $(CFLAGS)
bin/dragonfruit.so: $(OBJ)
	gcc $^ -o $@ $(CFLAGS) -shared
bin/%.o : %.c
	@mkdir -p bin
	gcc -c $< -o $@ $(INCLUDES) $(CFLAGS)

.PHONY: install test test_debug test_valgrind
install:bin/dragonfruit
	sudo install $^ $(DESTDIR)

clean:
	@rm -rv bin/

test:
ifeq ($(ANTECEDENT_EXISTS),1)
	@echo Testing...
	@if ./bin/dragonfruit $(ANTECEDENT_PATH)/src/Antecedent.d bin/test.asm; then echo Compiled successfully;else echo Failed to compile;fi
else
	@echo Path specified does not exist!
endif

test_debug:
ifeq ($(ANTECEDENT_EXISTS),1)
	@echo Testing...
	@gdb --args ./bin/dragonfruit $(ANTECEDENT_PATH)/src/Antecedent.d bin/test.asm
else
	@echo Path specified does not exist!
endif


test_valgrind:
ifeq ($(ANTECEDENT_EXISTS),1)
	@echo Testing...
	@valgrind --leak-check=full -v ./bin/dragonfruit $(ANTECEDENT_PATH)/src/Antecedent.d bin/test.asm
else
	@echo Path specified does not exist!
endif
