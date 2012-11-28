#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>	

static int __init init_mutex(void)
{
	printk("Hello world 1.\n");

	return 0;
}

static void __exit mutex_cleanup(void)
{
	printk("Goodbye world 1.\n");
}


module_init(init_mutex);
module_exit(mutex_cleanup);