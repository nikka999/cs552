#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/errno.h>

void my_printk(char *string);

static int __init init_mutex(void)
{
	my_printk("Hello world 1.\n");

	return 0;
}

static void __exit mutex_cleanup(void)
{
	my_printk("Goodbye world 1.\n");
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

module_init(init_mutex);
module_exit(mutex_cleanup);