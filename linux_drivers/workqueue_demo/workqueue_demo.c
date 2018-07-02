/*
 *  Driver to demonstrate implementation and properties
 *  of linux workqueues.
 *
 *  Author - Sharvil Shah <spshah16@asu.edu>
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include "workqueue_demo.h"

struct work_struct *led_work;


/*
 *	interrupt handler
 *
 *	Details:
 *		- called when a rising edge on GPIO1 generates an interrupt signal
 *		- schedules preinitialized work
 *		- returns IRQ_HANDLED
 */
static irqreturn_t
interrupt_handler(int irq, void *dev_id)
{
	static int count = 1;
	schedule_work(led_work);
	printk(KERN_INFO "\t interrupt handled, count: %d\n", count);
	count++;
	return IRQ_HANDLED;
}


/*
 *	thread_function
 *
 *	Details:
 *		- called when a rising edge of a signal at GPIO1 generates
 *		an interrupt signal
 *		- schedules preinitialized work
 *		- returns IRQ_HANDLED
 */
static void thread_function(struct work_struct *work_arg)
{
	static int count = 1;
	gpio_set_value(led, 1);
	mdelay(200);
	gpio_set_value(led, 0);
	printk(KERN_INFO "\t work completed, count: %d\n", count);
	count++;
	return;
}


/*
 *	struct demo_dev
 *
 *	@cdev 	: struct cdev object
 *	@irq 	: irq line number
 */
struct demo_dev
{
	struct cdev cdev;
	u16	irq;
}
*dev_ptr;

static dev_t  demo_dev_num;
struct class* demo_class;


/*
 *	demo_dev_open - opens "demo_dev" for file operations
 *
 *	Details:
 *		- requests required gpio pins
 *		- requests irq line and registers interrupt_handler
 *		- returns 0 on success, error otherwise
 */
static int
demo_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0, irq_line = 0;

	dev_ptr = container_of(inode->i_cdev, struct demo_dev, cdev);
	filp->private_data = dev_ptr;

	gpio_free(led);
	gpio_free(led_ls);

	gpio_request(led,   "led");
	gpio_export(led,    false);
	gpio_export(led_ls, false);
	gpio_direction_output(led,    1);
	gpio_direction_output(led_ls, 0);
	gpio_set_value(led,  0);

	gpio_free(GPIO2);
	gpio_free(GPIO1);
	gpio_free(GPIO2_LEV);
	gpio_free(GPIO1_LEV);

	gpio_request(GPIO2, "trigger");
	gpio_export(GPIO2,     false);
	gpio_export(GPIO2_LEV, false);
	gpio_direction_output(GPIO2,     1);
	gpio_direction_output(GPIO2_LEV, 0);
	gpio_set_value(GPIO2, 0);

	gpio_request(GPIO1, "interrupt");	
	gpio_export(GPIO1,     false);
	gpio_export(GPIO1_LEV, false);
	gpio_direction_input(GPIO1);
	gpio_direction_input(GPIO1_LEV);

	/* get irq line # from gpio */
	irq_line = gpio_to_irq(GPIO1);
	if(irq_line < 0)
	{
		printk(KERN_INFO "gpio%d cannot be used as interrupt", GPIO1);
		return -EINVAL;
	}

	dev_ptr->irq = irq_line;

	/* register interrupt handler function */
	ret = request_irq(irq_line,
			interrupt_handler,
			IRQF_TRIGGER_RISING,
			"irq_test_device",
			dev_ptr);
	if(ret)
	{
		printk(KERN_INFO "unable to claim irq %d\n", irq_line);
		return ret;
	}

	led_work = kmalloc(sizeof(*led_work), GFP_KERNEL);

	INIT_WORK(led_work, thread_function);

	printk(KERN_INFO "%s: open()\n", DEVICE_NAME);
	return 0;
}


/*
 *	demo_dev_release - closes "demo_dev"
 *
 *	Details:
 *		- frees gpio pins and irq line requested by demo_dev_open
 *		- returns 0 on success
 */
static int
demo_dev_release(struct inode *inode, struct file *filp)
{
	gpio_free(led);
	gpio_free(led_ls);

	gpio_free(GPIO1);
	gpio_free(GPIO2);
	gpio_free(GPIO1_LEV);
	gpio_free(GPIO2_LEV);

	free_irq(dev_ptr->irq, dev_ptr);
	printk(KERN_INFO "%s: close()\n", DEVICE_NAME);
	return 0;
}


/*
 *	demo_dev_write
 * 
 *	Details:
 *		- generates a square wave signal at GPIO2 
 *		- returns 0 on success
 */
static int i = 0;
static ssize_t
demo_dev_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
	printk(KERN_INFO "** triggering interrupt %d **\n", i + 1);

	gpio_set_value(GPIO2, 1);
	mdelay(80);
	gpio_set_value(GPIO2, 0);
	mdelay(80);
	i++;
	return 0;
}


/*
 * file operations structure for the device
 */
static struct file_operations led_driver_fops = 
{
	.owner			= THIS_MODULE,
	.open 			= demo_dev_open,
	.release 		= demo_dev_release,
	.write 			= demo_dev_write,
};


/*
 *	demo_dev_init
 *
 *	Details:
 *		- called when module loaded into kernel
 *		- initializes "demo_dev" device
 *		- returns  on success, an error code otherwise
 */
static int 
__init demo_module_init(void)
{
	int ret = 0;

	/* allocate device numbers dynamically */
	if(alloc_chrdev_region(&demo_dev_num, 0, 1, DEVICE_NAME) < 0)
	{
		printk(KERN_INFO "Cannot register char device numbers\n");
		return -1;
	}
	printk(KERN_INFO "<Major, Minor>: <%d, %d>\n",
			MAJOR(demo_dev_num), MINOR(demo_dev_num));

	demo_class = class_create(THIS_MODULE, DRIVER_NAME);

	/* allocate memory for the demo_dev structure */
	dev_ptr = kzalloc(sizeof(struct demo_dev), GFP_KERNEL);
	if(!dev_ptr)
	{
		printk(KERN_INFO "Bad kzalloc\n");
		return -ENOMEM;
	}

	/* connect file operations with cdev */
	cdev_init(&dev_ptr->cdev, &led_driver_fops);
	dev_ptr->cdev.owner = THIS_MODULE;

	/* connect major and minor numbers to cdev */
	ret = cdev_add(&dev_ptr->cdev,
			MKDEV(MAJOR(demo_dev_num), MINOR(demo_dev_num)), 1);
	if(ret)
	{
		printk(KERN_INFO "Bad cdev\n");
		return ret;
	}

	/* create device */
	device_create(demo_class, NULL,
			MKDEV(MAJOR(demo_dev_num), MINOR(demo_dev_num)),
			NULL, "%s", DEVICE_NAME);

	printk(KERN_INFO "%s: module loaded\n", DEVICE_NAME);
	return 0;
}
module_init(demo_module_init);


/*
 *	demo_dev_exit - called when module unloaded from kernel
 *
 *	Details:
 *		- releases device numbers and kernel memory
 */
static void 
__exit demo_module_exit(void)
{
	/* free device number */
	unregister_chrdev_region(demo_dev_num, 1);

	/* destroy device */
	device_destroy(demo_class, MKDEV(MAJOR(demo_dev_num), 0));
	cdev_del(&dev_ptr->cdev);
	kfree(dev_ptr);

	/* destroy driver class */
	class_destroy(demo_class);

	printk(KERN_INFO "%s: module unloaded\n", DEVICE_NAME);
}
module_exit(demo_module_exit);


MODULE_AUTHOR("Sharvil Shah");
MODULE_LICENSE("GPL v2");
