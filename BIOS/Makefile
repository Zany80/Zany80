AS=scas
CC=kcc
LD=scas
LDFLAGS=-fno-remove-unused-funcs
ASFLAGS=-fexplicit-export -fexplicit-import -Iinclude -fno-remove-unused-funcs
CFLAGS=-Iinclude --no-std-crt0 --std-c11 -mz80 #--nostdinc --nostdlib 

bin/src/%.o:src/%.asm
	mkdir $(dir $@) -p
	$(AS) $(ASFLAGS) -c $< -o $@

bin/src/%.o:src/%.c
	mkdir $(dir $@) -p
	$(CC) -S $< -o $(basename $@).asm $(CFLAGS)
	$(AS) $(ASFLAGS) -c $(basename $@).asm -o $@
#	rm $(basename $@).asm

BANK1=$(addprefix bin/, $(addsuffix .o, $(basename $(wildcard src/bank1/*))))
BANK2=$(addprefix bin/, $(addsuffix .o, $(basename $(wildcard src/bank2/*))))
BANK3=$(addprefix bin/, $(addsuffix .o, $(basename $(wildcard src/bank3/*))))
OBJECTS=$(BANK1) $(BANK2) $(BANK3)

BANKS = bin/bank1.rom bin/bank2.rom bin/bank3.rom

default:$(BANKS)

bin/bank1.rom: $(BANK1)
	$(LD) $(LDFLAGS) -forigin=0x0000 $^ -o $@

bin/bank2.rom: $(BANK2)
	$(LD) $(LDFLAGS) -forigin=0x4000 $^ -o $@

bin/bank3.rom: $(BANK3)
	echo $(BANK3)
	$(LD) $(LDFLAGS) -forigin=0x8000 $^ -o $@

clean:
	$(RM) $(LIB) $(BANKS) $(OBJECTS)
