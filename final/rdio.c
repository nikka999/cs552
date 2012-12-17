#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "rdio.h"

#include <stdio.h>

int rd_creat(char *pathname) {
	int fd = open("/proc/ramdisk", O_RDONLY);
	printf("creat: this is pathname: %s\n", pathname);
	int rc = ioctl(fd, RD_CREAT, pathname);
	close(fd);
	return rc;
}

int rd_mkdir(char *pathname) {
	int fd = open("/proc/ramdisk", O_RDONLY);
	printf("mkdir: this is pathname: %s\n", pathname);
	int rc = ioctl(fd, RD_MKDIR, pathname);
	close(fd);
	return rc;	
}

int rd_open(char *pathname) {
	int fd = open("/proc/ramdisk", O_RDONLY);
	printf("open: this is pathname: %s\n", pathname);
	int rc = ioctl(fd, RD_OPEN, pathname);
	close(fd);
	return rc;
}

int rd_close(int fd) {
	int sd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(sd, RD_CLOSE, &fd);
	close(sd);
	return rc;
}

int rd_read(int fd, char *address, int num_bytes) {
	Params p;
	p.fd = fd;
	p.addr = address;
	p.num_bytes = num_bytes;
	int sd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(sd, RD_READ, &p);
	close(sd);
	return rc;
}

int rd_write(int fd, char *address, int num_bytes) {
	Params p;
	p.fd = fd;
	p.addr = address;
	p.num_bytes = num_bytes;
	int sd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(sd, RD_WRITE, &p);
	close(sd);
	return rc;
}

int rd_lseek(int fd, int offset) {
	Params p;
	p.fd = fd;
	p.addr = NULL;
	p.num_bytes = offset;
	int sd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(sd, RD_LSEEK, &p);
	close(sd);
	return rc;	
}

int rd_unlink(char *pathname) {
	int fd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(fd, RD_UNLINK, pathname);
	close(fd);
	return rc;
}

int rd_readdir(int fd, char *address) {
	Params p;
	p.fd = fd;
	p.addr = address;
	p.num_bytes = 0;
	int sd = open("/proc/ramdisk", O_RDONLY);
	int rc = ioctl(sd, RD_READDIR, &p);
	close(sd);
	return rc;
}
