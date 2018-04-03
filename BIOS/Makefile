AS=scas
CC=kcc
LD=scas
LDFLAGS=-fno-remove-unused-funcs
ASFLAGS=-fexplicit-export -fexplicit-import -Iinclude -fno-remove-unused-funcs
CFLAGS=-Iinclude --no-std-crt0 --std-c11 -mz80 #--nostdinc --nostdlib 

bin/src/%.o:src/%.asm
	mkdir $(dir $@) -p
	$(AS) $(ASFLAGS) -c $< -o $@

bin/src/%.o:src/%.c include
	mkdir $(dir $@) -p
	$(CC) -S $< -o $(basename $@).asm $(CFLAGS)
	$(AS) $(ASFLAGS) -c $(basename $@).asm -o $@
#	rm $(basename $@).asm

OS=$(addprefix bin/, $(addsuffix .o, $(basename src/os/crt0.asm $(wildcard src/os/*.c))))
BIOS_SOURCES=src/bios/bios.asm $(wildcard src/bios/*.c) $(wildcard src/bios/*.asm) $(wildcard src/bios/fs/*.c)
BIOS=$(addprefix bin/, $(addsuffix .o, $(basename $(BIOS_SOURCES))))
OBJECTS=$(OS) $(BIOS)

default:bin/os.rom bin/bios.rom

bin/os.rom: $(OS)
	$(LD) $(LDFLAGS) -forigin=0x0000 $^ -o $@

bin/bios.rom: $(BIOS)
	$(LD) $(LDFLAGS) -forigin=0x8000 $^ -o $@

clean:
	$(RM) $(LIB) $(BANKS) $(OBJECTS)
