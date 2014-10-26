#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#define test_dbg(fmt,arg...) printk("%s:%d: "fmt, __FUNCTION__, __LINE__, ##arg)

static ssize_t test_module_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos)
{
	test_dbg();


	test_dbg();
	return 1;
}

static ssize_t test_module_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	test_dbg();


	test_dbg();
	return 1;
}				

static const struct file_operations test_module_fops = {
	.owner		= THIS_MODULE,
	.write		= test_module_write,
	.read		= test_module_read
};

static struct miscdevice test_module_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "test_module",
	.fops	= &test_module_fops,
};


static int __init test_module_init(void)
{

	test_dbg("test_module_init ...\n");
	return misc_register(&test_module_miscdev);
}

static void __exit test_module_exit(void)
{	
	test_dbg("test_module_exit ...\n");
	misc_deregister(&test_module_miscdev);
}

module_init(test_module_init);
module_exit(test_module_exit);

MODULE_AUTHOR("JianTu");
MODULE_DESCRIPTION("test_module");
MODULE_LICENSE("GPL");

