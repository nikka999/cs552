all: kernel.bin

kernel.bin: kernel.o loader.o
	ld -T linker.ld -o kernel.bin loader.o kernel.o

kernel.o: kernel.c
	gcc -o kernel.o kernel.c -nostdlib -fno-builtin -nostartfiles -nodefaultlibs

loader.o: loader.s
	as -o loader.o loader.s

clean:
	rm kernel.o kernel.bin loader.o
