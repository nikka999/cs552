obj-m += ramd.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o test test.c rdio.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm test
