#ifndef __UDEBUG_H
#define __UDEBUG_H

#include <linux/module.h>
#include <linux/device.h>

#define KOBJ_ATTR_UL(name) \
static volatile unsigned int name = 0; \
static ssize_t name##_show(struct kobject *kobj, \
			struct kobj_attribute *attr, char *buf) \
{ \
	return snprintf(buf, 256, #name"=0x%x\n", name); \
	return 0; \
} \
 \
static ssize_t name##_store(struct kobject *kobj, struct kobj_attribute *attr, \
			 const char *buf, size_t count) \
{ \
	name  = simple_strtoul(buf, NULL, 16); \
	return count; \
} \
 \
static struct kobj_attribute name##_attr = \
	__ATTR(name, S_IRUGO|S_IWUSR, name##_show, name##_store);

#define DEV_ATTR_UL(name) \
static volatile unsigned int name; \
static ssize_t name##_show(struct device *_dev, \
	struct device_attribute *attr, char *buf) \
{ \
	return snprintf(buf, 256, #name"=0x%x\n", name); \
	return 0; \
} \
 \
static ssize_t name##_store( struct device *_dev, \
	struct device_attribute *attr, const char *buf, size_t count) \
{ \
	name  = simple_strtoul(buf, NULL, 16); \
	return count; \
} \
 \
static DEVICE_ATTR(xstart, S_IRUGO|S_IWUSR, (void *)name##_show, (void *)name##_store);

#endif

