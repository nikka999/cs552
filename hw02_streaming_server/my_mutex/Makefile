obj-m += my_mutex.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o mutex_calls mutex_calls.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mutex_calls
