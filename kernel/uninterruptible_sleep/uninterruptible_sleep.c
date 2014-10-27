#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/miscdevice.h>

static DECLARE_COMPLETION(done);

static ssize_t rcu_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos)
{
	init_completion(&done);
	printk("rcu_read ...in\n");
    wait_for_completion(&done);
	printk("rcu_read ...out\n");
	return 1;
}

static ssize_t rcu_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	printk("rcu_write in\n");
	complete(&done);
	printk("rcu_write out\n");
	return 1;
}				

static const struct file_operations rcu_fops = {
	.owner		= THIS_MODULE,
	.write		= rcu_write,
	.read		= rcu_read
};

static struct miscdevice rcu_miscdev = {
	.minor	= WATCHDOG_MINOR,
	.name	= "rcu",
	.fops	= &rcu_fops,
};


static int __init rcu_test_init(void)
{

	printk("rcu_test_init ...\n");
	return misc_register(&rcu_miscdev);
}

static void __exit rcu_test_exit(void)
{	
	printk("rcu_test_exit ...\n");
}

module_init(rcu_test_init);
module_exit(rcu_test_exit);

MODULE_AUTHOR("JianTu");
MODULE_DESCRIPTION("rcu test module");
MODULE_LICENSE("GPL");
