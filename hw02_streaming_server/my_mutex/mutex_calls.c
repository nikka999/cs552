#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/semaphore.h>

#define MUTEX_LOCK _IOW(0, 0, struct semaphore)

//void my_mutex_lock()

struct semaphore {
	void *sem;
};


int main () {

  /* attribute structures */
  // struct ioctl_test_t {
  //   int field1;
  //   char field2;
  // } ioctl_test;

  	int fd = open ("/proc/my_mutex", O_RDONLY);

  // ioctl_test.field1 = 10;
  // ioctl_test.field2 = 'a';
	struct semaphore sem;

  	ioctl (fd, MUTEX_LOCK, &sem);

  	return 0;
}
