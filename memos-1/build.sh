as --32 memos-1.s -o memos-1.o
ld -T memos-1.ld memos-1.o -o memos-1
dd bs=1 if=memos-1 of=memos-1_test skip=4096 count=512
sudo qemu memos-1_test
