#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/semaphore.h>

#define MUTEX_LOCK _IOW(0, 0, struct ioctl_test_t)

void my_mutex_lock()




int main () {

  /* attribute structures */
  // struct ioctl_test_t {
  //   int field1;
  //   char field2;
  // } ioctl_test;

  	int fd = open ("/proc/my_mutex", O_RDONLY);

  // ioctl_test.field1 = 10;
  // ioctl_test.field2 = 'a';
	sturct semaphore sem;

  	ioctl (fd, MUTEX_LOCK, &sem);

  	return 0;
}
