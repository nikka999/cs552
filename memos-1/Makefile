all: memos-1_test

memos-1_test: memos-1
	dd bs=1 if=memos-1 of=memos-1_test skip=4096 count=512

memos-1: memos-1.o memos-1.ld
	ld -T memos-1.ld memos-1.o -o memos-1

memos-1.o: memos-1.s
	as --32 memos-1.s -o memos-1.o

clean:
	rm -rf memos-1.o memos-1_test memos-1
