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
#include <linux/miscdevice.h>
#include <linux/miscdevice.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/pipe_fs_i.h>
#include <linux/swap.h>
#include <linux/splice.h>
#include <linux/aio.h>
#include <linux/freezer.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

volatile static int flag;
//static wait_queue_head_t waitq;
static DECLARE_WAIT_QUEUE_HEAD(waitq);

static ssize_t rcu_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos)
{
	int ret = 0;
	flag = 0;
	printk("%s: %d\n", __FUNCTION__, __LINE__);

	while (flag != 1) {
		ret = wait_event_freezable(waitq, flag == 1);
//		printk_ratelimit
		printk("ret = %d\n", ret);
	}
				     
	printk("%s: %d\n", __FUNCTION__, __LINE__);
	return 1;
}

static ssize_t rcu_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	printk("%s: %d\n", __FUNCTION__, __LINE__);

	flag = 1;
	wake_up(&waitq);
	printk("%s: %d\n", __FUNCTION__, __LINE__);

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
