gcc -o kernel.o kernel.c -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
ld -T linker.ld -o kernel.bin loader.o kernel.o
sudo mount /home/nikka/Documents/cs552/part2/c.img /mnt/C/ -text2 -o loop,offset=32256
sudo rm /mnt/C/boot/kernel.bin
sudo cp /home/nikka/Documents/cs552/bare_bone/kernel.bin /mnt/C/boot/
sudo umount /mnt/C/
