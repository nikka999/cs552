#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include "ramd.h"

MODULE_LICENSE("GPL");

static unsigned char *ramdisk;
static struct proc_dir_entry *proc_entry;
static int ramdisk_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static struct file_operations proc_operations;

struct Params {
	int fd;
	char* addr;
	int num_bytes;
};

static int __init init_routine(void) {
	printk("<1> Loading RAMDISK Module\n");

	proc_operations.ioctl = ramdisk_ioctl;

	// Create /proc entry first. 666 for Read/Write
	proc_entry = create_proc_entry("ramdisk", 0666, NULL);
	if (!proc_entry) {
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}
	// Working version
	proc_entry->proc_fops = &proc_operations;

// Trying read write
//	proc_entry->read_proc = read_proc;
//	proc_entry->write_proc = write_proc;

	// vmalloc for 2MB
	// ramdisk = (char *)vmalloc(2097150);

	return 0;
}

static void __exit exit_routine(void) {
	printk("<1> Exiting RAMDISK Module\n");
	// Free ramdisk
	vfree(ramdisk);

	// Remove /proc entry
	remove_proc_entry("ramdisk", NULL);
	return;
}

/** 
* Ramdisk entry point
*/

static int ramdisk_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	int fd;
	// int size;
	unsigned long size;
	char *pathname;
	struct Params p;
	char blah[8] = "bigfoot";
	/* 
	 * Switch according to the ioctl called 
	 */
	switch (cmd) {
	case RD_MALLOC:

		// size = (int)arg;
		// 
		// get_user(ch, temp);
		// for (i = 0; ch && i < BUF_LEN; i++, temp++)
		// 	get_user(ch, temp);
		// 
		// device_write(file, (char *)ioctl_param, i, 0);
		// vmalloc for 2MB
		ramdisk = (unsigned char *)vmalloc(2097150);
		printk("<1>I finished vmalloc!\n");
		break;
	case RD_CREAT:
		size = strnlen_user((char *)arg, 50);
		pathname = (char *)kmalloc(size,GFP_KERNEL);
		copy_from_user(pathname, (char *)arg, size);
		printk("<1> kernel got: %s\n",pathname);
		printk("<1> the len is %lu\n", size);
		kfree(pathname);
		return 0;
		break;
	case RD_MKDIR:
		size = strnlen_user((char *)arg, 50);
		pathname = (char *)kmalloc(size,GFP_KERNEL);
		copy_from_user(pathname, (char *)arg, size);
		printk("<1> kernel got: %s\n",pathname);
		printk("<1> the len is %lu\n", size);
		kfree(pathname);
		return 0;
		break;
	case RD_OPEN:
		size = strnlen_user((char *)arg, 50);
		pathname = (char *)kmalloc(size,GFP_KERNEL);
		copy_from_user(pathname, (char *)arg, size);
		printk("<1> kernel got: %s\n",pathname);
		printk("<1> the len is %lu\n", size);
		kfree(pathname);
		return 0;
		break;
	case RD_CLOSE:
		get_user(fd, (int *)arg);
		printk("<1> kernel: the fd is %d\n", fd);
		break;
	case RD_READ:
		copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
		printk("<1> got p.fd:%d, p.addr: %p, p.byte_size:%d\n", p.fd, p.addr, p.num_bytes);
		copy_to_user(p.addr, blah, 8); 
		return 0;
		break;
	case RD_WRITE:
		copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
		printk("<1> got p.fd:%d, p.addr: %p, p.byte_size:%d\n", p.fd, p.addr, p.num_bytes);
		copy_to_user(p.addr, blah, 8); 
		return 0;
		break;
	case RD_LSEEK:
		copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
		printk("<1> got p.fd:%d, p.byte_size:%d\n", p.fd, p.num_bytes);
		// copy_to_user(p.addr, blah, 8); 
		return 0;
		break;
	case RD_UNLINK:
		size = strnlen_user((char *)arg, 50);
		pathname = (char *)kmalloc(size,GFP_KERNEL);
		copy_from_user(pathname, (char *)arg, size);
		printk("<1> kernel got: %s\n",pathname);
		printk("<1> the len is %lu\n", size);
		kfree(pathname);
		return 0;
		break;
	case RD_READDIR:
		copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
		printk("<1> got p.fd:%d, p.addr: %p\n", p.fd, p.addr);
		copy_to_user(p.addr, blah, 8); 
		return 0;
		break;
	}

	return 0;
}

module_init(init_routine);
module_exit(exit_routine);
