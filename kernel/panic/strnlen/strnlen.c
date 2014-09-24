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

#define S3G_DRV_DEBUG     0x00
#define S3G_DRV_WARNING   0x01
#define S3G_DRV_INFO      0x02
#define S3G_DRV_ERROR     0x03
#define S3G_DRV_EMERG     0x04

#define s3g_emerg(args...)   s3g_printk(S3G_DRV_EMERG, ##args)
#define s3g_error(args...)   s3g_printk(S3G_DRV_ERROR, ##args)
#define s3g_info(args...)    s3g_printk(S3G_DRV_INFO,  ##args)
#define s3g_warning(args...) s3g_printk(S3G_DRV_WARNING, ##args)

static unsigned char* s3g_msg_prefix_string[] = 
{
    "s3g debug",
    "s3g warning",
    "s3g info",
    "s3g error",
    "s3g emerg"
};

void  s3g_printk(unsigned int msglevel, const char* fmt, ...)
{
    char buffer[256];
    va_list marker;

    if(1)
    {
        va_start(marker, fmt);
        vsnprintf(buffer, 256, fmt, marker);
        va_end(marker);

        switch ( msglevel )
        {
        case S3G_DRV_DEBUG:
            printk(KERN_DEBUG"%s: %s", s3g_msg_prefix_string[msglevel], buffer);
        break;
        case S3G_DRV_WARNING:
            printk(KERN_WARNING"%s: %s", s3g_msg_prefix_string[msglevel], buffer);
        break;
        case S3G_DRV_INFO:
            printk(KERN_INFO"%s: %s", s3g_msg_prefix_string[msglevel], buffer);
        break;
        case S3G_DRV_ERROR:
            printk(KERN_ERR"%s: %s", s3g_msg_prefix_string[msglevel], buffer);
        break;
        case S3G_DRV_EMERG:
            printk(KERN_EMERG"%s: %s", s3g_msg_prefix_string[msglevel], buffer);
        break;
        default:
            /* invalidate message level */
           printk("ddddddddddddd");
        break;
        }
    }
}


static ssize_t rcu_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos)
{
	printk("rcu_read ...11\n");
	printk("%s rcu_read ...22\n");
	return 1;
}

static ssize_t rcu_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	printk("rcu_write ...11\n");
	s3g_error("%sfdjsalkj\n");
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
