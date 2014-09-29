/*
 * Copyright (c) 2000-2004 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/sysdev.h>
#include <mach/comdef.h>
#include <mach/map.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/device.h>
#include"udebug.h"

#define PHYADDR_MIN 0x0
#define PHYADDR_MAX 0xffffffff

static struct kobject *debug_kobj;

KOBJ_ATTR_UL(phyaddr);
KOBJ_ATTR_UL(length);

static ssize_t mem_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	void __iomem *vaddr = NULL;
	unsigned int tempaddr, templength;
	int i;

	if(phyaddr > PHYADDR_MAX || phyaddr < PHYADDR_MIN || 0 != (phyaddr & 0x3))
		return -1;

	tempaddr = phyaddr;

	if(0 == length)
		templength = 4;
	else
		templength = (length + 3) & 0xfc;

	vaddr = ioremap_nocache(tempaddr, SZ_256);
	if(vaddr)
	{
		for(i = 0; i < templength; i = i + 4)
			printk("reg[0x%x] = 0x%x\n", tempaddr + i, readl(vaddr + i));

		iounmap(vaddr);
	}

	return 0;
}

static ssize_t mem_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned int temp  = simple_strtoul(buf, NULL, 16);
	void __iomem *vaddr = NULL;
	unsigned int tempaddr;

	if(phyaddr > PHYADDR_MAX || phyaddr < PHYADDR_MIN || 0 != (phyaddr & 0x3))
		return -1;

	tempaddr = phyaddr;

	vaddr = ioremap_nocache(tempaddr, SZ_16);
	if(vaddr)
	{
		writel(temp, vaddr);
		printk("write 0x%x  -->> reg[0x%x]\n", temp, tempaddr);
		iounmap(vaddr);
	}

	return count;
}

static struct kobj_attribute mem_attr =
	__ATTR(mem, S_IRUGO | S_IWUSR, mem_show, mem_store);

static int __init init(void)
{
	printk("udebug init ... \n");

	debug_kobj = NULL;

	 debug_kobj = kobject_create_and_add("debug", NULL);
	if(debug_kobj)
	{
		sysfs_create_file(debug_kobj, &mem_attr.attr);	
		sysfs_create_file(debug_kobj, &phyaddr_attr.attr);	
		sysfs_create_file(debug_kobj, &length_attr.attr);	
		return 0;
	}

	return -1;
}

static void __exit cleanup(void)
{
	printk("udebug exit ... \n");

	sysfs_remove_file(debug_kobj, &mem_attr.attr);	
	sysfs_remove_file(debug_kobj, &phyaddr_attr.attr);	
	sysfs_remove_file(debug_kobj, &length_attr.attr);	

	kobject_put(debug_kobj);
}

module_init(init);
module_exit(cleanup);

MODULE_DESCRIPTION("user debug module");
MODULE_LICENSE("GPL");

