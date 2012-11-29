#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/errno.h>

void my_printk(char *string);

struct ioctl_test_t {
  int field1;
  char field2;
};

#define MUTEX_LOCK _IOW(0, 6, struct ioctl_test_t)

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
                               unsigned int cmd, unsigned long arg);

static struct file_operations pseudo_dev_proc_operations;

static struct proc_dir_entry *proc_entry;

static int __init init_mutex(void)
{
	my_printk("Loading my_mutex module\n");
	pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;
	
	proc_entry = create_proc_entry("my_mutex", 0444, NULL);
	if(!proc_entry)
  	{
    	my_printk("Error creating /proc entry.\n");
    	return 1;
  	}
  	proc_entry->proc_fops = &pseudo_dev_proc_operations;
	return 0;
}

static void __exit mutex_cleanup(void)
{
	my_printk("Removing my_mutex module\n");
	remove_proc_entry("my_mutex", NULL);
}

void my_printk(char *string)
{
  struct tty_struct *my_tty;

  my_tty = current->signal->tty;

  if (my_tty != NULL) {
    (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
    (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
  }
}

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
                                unsigned int cmd, unsigned long arg)
{
	struct ioctl_test_t ioc;

  	switch (cmd){

  		case MUTEX_LOCK:
    		copy_from_user(&ioc, (struct ioctl_test_t *)arg,
				sizeof(struct ioctl_test_t));
    		printk("ioctl: call to MUTEX_LOCK (%d,%c)!\n",
           		ioc.field1, ioc.field2);

    		my_printk ("Got msg in kernel\n");
    		break;

  		default:
    		return -EINVAL;
    		break;
  	}

  	return 0;
}
module_init(init_mutex);
module_exit(mutex_cleanup);