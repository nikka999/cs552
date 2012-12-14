#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define MAJOR_NUM 155

#define RD_MALLOC _IOR(MAJOR_NUM, 0, int)
#define RD_CREAT _IOR(MAJOR_NUM, 1, char *)
#define RD_MKDIR _IOR(MAJOR_NUM, 2, char *)
#define RD_OPEN _IOR(MAJOR_NUM, 3, char *)
#define RD_CLOSE _IOR(MAJOR_NUM, 4, int) 
#define RD_READ _IOR(MAJOR_NUM, 5, struct Params) //param data structure
#define RD_WRITE _IOR(MAJOR_NUM, 6, struct Params) //param ds
#define RD_LSEEK _IOR(MAJOR_NUM, 7, struct Params) //param ds
#define RD_UNLINK _IOR(MAJOR_NUM, 8, char *)
#define RD_READDIR _IOR(MAJOR_NUM, 9, struct Params) //param ds

int main () {

  /* attribute structures */
  // struct ioctl_test_t {
  //   int field1;
  //   char field2;
  // } ioctl_test;

	typedef struct Params {
		int fd;
		char *addr;
		int num_bytes;
	} Params;

  int fd = open ("/proc/ramdisk", O_RDONLY);

  // ioctl_test.field1 = 10;
  // ioctl_test.field2 = 'a';
	char face[10];
	int sd = 8;
	int bz = 10;
	Params p;
	p.fd = sd;
	p.addr = face;
	p.num_bytes = bz;
	char *s;
	s = "very nice!!";
	printf("about to send out %s\n",s);
	int rc;	
  	rc = ioctl (fd, RD_CLOSE, &sd);
	printf("this was the return: %d\n",rc);
	// printf("this is p.addr: %s\n", p.addr);

  return 0;
}

