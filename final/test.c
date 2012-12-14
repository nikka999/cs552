#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define MAJOR_NUM 155

#define RD_CREAT _IOR(MAJOR_NUM, 1, char *)

int main () {

  /* attribute structures */
  // struct ioctl_test_t {
  //   int field1;
  //   char field2;
  // } ioctl_test;

  int fd = open ("/proc/ramdisk", O_RDONLY);

  // ioctl_test.field1 = 10;
  // ioctl_test.field2 = 'a';
	char *s;
	s = "hello!";
	
  ioctl (fd, RD_CREAT, s);

  return 0;
}

