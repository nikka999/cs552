#define MAJOR_NUM 155

// #define RD_MALLOC _IOR(MAJOR_NUM, 0, int)
#define RD_CREAT _IOR(MAJOR_NUM, 0, char *)
#define RD_MKDIR _IOR(MAJOR_NUM, 1, char *)
#define RD_OPEN _IOR(MAJOR_NUM, 2, char *)
#define RD_CLOSE _IOR(MAJOR_NUM, 3, int) 
#define RD_READ _IOWR(MAJOR_NUM, 4, struct Params) //param data structure
#define RD_WRITE _IOWR(MAJOR_NUM, 5, struct Params) //param ds
#define RD_LSEEK _IOR(MAJOR_NUM, 6, struct Params) //param ds
#define RD_UNLINK _IOR(MAJOR_NUM, 7, char *)
#define RD_READDIR _IOWR(MAJOR_NUM, 8, struct Params) //param ds

typedef struct Params {
	int fd;
	char *addr;
	int num_bytes;
} Params;

int rd_creat(char *pathname);
int rd_mkdir(char *pathname);
int rd_open(char *pathname);
int rd_close(int fd);
int rd_read(int fd, char *address, int num_bytes);
int rd_write(int fd, char *address, int num_bytes);
int rd_lseek(int fd, int offset);
int rd_unlink(char *pathname);
int rd_readdir(int fd, char *address);
