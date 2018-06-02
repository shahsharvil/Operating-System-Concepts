/*************************************************************************

File Name: rgbled.c

Description: A character device driver to control the intensity of an
			 RGB-Led using pulse width modulation and a high resolution
			 timer.

Author: Sharvil Shah (spshah16@asu.edu)

*************************************************************************/

#include <linux/module.h>
#include <linux/init.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>		/* error codes */
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>			/* kmalloc() */

#include "rgbled.h"				  /* local headers */

static dev_t dev_num;
struct class *dev_class;
struct led_dev *dev_ptr;

static struct hrtimer my_hrt;
static ktime_t k_per_ns;		/* stores on_time_ns or off_time_ns */


MODULE_AUTHOR("Sharvil Shah");
MODULE_LICENSE("GPL v2");


/*************************************************************************
timer_expiry_cb_function

Input Arguments:
	1. timer  - pointer to struct hrtimer

Description:
	1. Called when timer expires for the first time.
	2. Toggles RGB pin values between 1 and 0 at a specific frequency
	   to produce desired intensity levels

Return Value: HRTIMER_RESTART or HRTIMER_NORESTART

*************************************************************************/
static enum hrtimer_restart timer_expiry_cb_fun(struct hrtimer *timer)
{	
	static volatile int itr = 1;
	ktime_t ktime_now;
	
	if(itr < MAX)
	{	
		if(itr % 2 == 1)
		{
			ktime_now = hrtimer_cb_get_time(timer);

			gpio_set_value(dev_ptr->red,   0);
			gpio_set_value(dev_ptr->blue,  0);
			gpio_set_value(dev_ptr->green, 0);

			k_per_ns = ktime_set(0, dev_ptr->off_time_ns);
			hrtimer_forward(timer, ktime_now, k_per_ns);			
			itr++;

			return HRTIMER_RESTART;
		}
		else
		{	
			ktime_now = hrtimer_cb_get_time(timer);
			
			gpio_set_value(dev_ptr->red,   1);
			gpio_set_value(dev_ptr->blue,  1);
			gpio_set_value(dev_ptr->green, 1);
			
			k_per_ns = ktime_set(0, dev_ptr->on_time_ns);
			hrtimer_forward(timer, ktime_now, k_per_ns);
			itr++;
			
			return HRTIMER_RESTART;
		}		
	}
	else
	{	
		itr = 1;
		return HRTIMER_NORESTART;
	}
}


/*************************************************************************
led_driver_open

Input Arguments:
	1. inode - pointer to inode structure
	2. filp  - pointer to file

Description:
	1. Called when device opened.

Return Value: 0 on success 

*************************************************************************/
int
led_driver_open(struct inode *inode, struct file *filp)
{	
	printk(KERN_INFO "Driver: open()\n");
	dev_ptr = container_of(inode->i_cdev, struct led_dev, cdev);
	filp->private_data = dev_ptr;
	
	return 0;
}


/*************************************************************************
led_driver_release

Input Arguments:
	1. inode - pointer to inode structure
	2. filp  - pointer to file

Description:
	1. Called when device released.
	2. Frees the gpio pins previously requested.

Return Value: 0 on success 

*************************************************************************/
int
led_driver_release(struct inode *inode, struct file *filp)
{	
	printk("Driver: close()\n");
	
	/* free requested gpio pins */	
	gpio_free(dev_ptr->red);
	gpio_free(dev_ptr->green);
	gpio_free(dev_ptr->blue);
	
	gpio_free(dev_ptr->red_ls);
	gpio_free(dev_ptr->green_ls);
	gpio_free(dev_ptr->blue_ls);

	return 0;
}


/*************************************************************************
led_driver_ioctl

Input Arguments:
	1. filp  - pointer to file
	2. cmd   - command number (not needed)
	3. count - user space pointer to ioctl_message

Description:
	1. copies RGB gpio pin numbers and pwm value from user space
	2. exports corresponding pins and level shifters

Return Value: 0 on success, EINVAL on failure

*************************************************************************/
static long led_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long count)
{	
	struct ioctl_message *ptr;
	ptr = (struct ioctl_message *)kmalloc(sizeof(struct ioctl_message),GFP_KERNEL);	

	copy_from_user(ptr, (struct ioctl_message *)count, sizeof(struct ioctl_message));
	
	printk("Driver: ioctl(), r_pin: %d, g_pin: %d, b_pin: %d, pwm: %d\n", 
			    ptr->r_pin, ptr->g_pin, ptr->b_pin, ptr->pwm);

	if( (ptr->r_pin > 5 || ptr->r_pin > 5 || ptr->r_pin > 5) || (ptr->pwm > 100) )
	{	
		kfree(ptr);
		return EINVAL;
	}

	/* get linux i/o and level shifter pins */
	dev_ptr->red      = linux_io[ptr->r_pin];
	dev_ptr->red_ls   = level_sh[ptr->r_pin];

	dev_ptr->green    = linux_io[ptr->g_pin];
	dev_ptr->green_ls = level_sh[ptr->g_pin];
	
	dev_ptr->blue     = linux_io[ptr->b_pin];
	dev_ptr->blue_ls  = level_sh[ptr->b_pin];

	dev_ptr->pwm = ptr->pwm;
	
	/* compute led on and off times per cycle, as per the pwm (duty cycle) value */
	dev_ptr->on_time_ns =  (per_ns/100) * (dev_ptr->pwm);
	dev_ptr->off_time_ns = (per_ns/100) * (100 - dev_ptr->pwm);

	kfree(ptr);
	
	/* free, export and set directions of gpio pins*/
	gpio_free(dev_ptr->red);
	gpio_free(dev_ptr->green);
	gpio_free(dev_ptr->blue);
	
	gpio_free(dev_ptr->red_ls);
	gpio_free(dev_ptr->green_ls);
	gpio_free(dev_ptr->blue_ls);

	gpio_request(dev_ptr->red, "Red");
	gpio_export(dev_ptr->red,    false);
	gpio_export(dev_ptr->red_ls, false);	
	gpio_direction_output(dev_ptr->red,    1);
	gpio_direction_output(dev_ptr->red_ls, 0);

	gpio_request(dev_ptr->green, "Green");
	gpio_export(dev_ptr->green,    false);
	gpio_export(dev_ptr->green_ls, false);	
	gpio_direction_output(dev_ptr->green,    1);
	gpio_direction_output(dev_ptr->green_ls, 0);

	gpio_request(dev_ptr->blue, "Blue");
	gpio_export(dev_ptr->blue,    false);
	gpio_export(dev_ptr->blue_ls, false);	
	gpio_direction_output(dev_ptr->blue,    1);
	gpio_direction_output(dev_ptr->blue_ls, 0);

	return 0;
}


/*************************************************************************
led_driver_write

Input Arguments:
	1. filp  - pointer to file
	2. buf   - address of user space buffer
	3. count - size of buffer
	4. ppos  - position offset

Description:
	1. Sets RGB pins to 1.
	2. Starts high resolution timer

	(Note: When the high resolution timer expires for the first time,
	 	     the timer_expiry_cb_fun is called.)

Return Value: 0 on success 

*************************************************************************/
static ssize_t
led_driver_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{	
	k_per_ns = ktime_set(0, dev_ptr->on_time_ns);

	gpio_set_value(dev_ptr->red, 1);
	gpio_set_value(dev_ptr->blue, 1);
	gpio_set_value(dev_ptr->green, 1);

	hrtimer_start(&my_hrt, k_per_ns, HRTIMER_MODE_REL);

	return 0;
}


/*
 * File operations structure for the device
 */
static struct file_operations led_driver_fops = 
{
	.owner   = THIS_MODULE,
	.open    = led_driver_open,
	.release = led_driver_release,
	.write   = led_driver_write,
	.unlocked_ioctl	= led_driver_ioctl,
};


/*************************************************************************
pulse_driver_init

Description:
	1. Called when driver loaded into the kernel
	2. Initializes the Ultrasonic Distance Sensor

Return Value: 0 on success

*************************************************************************/
static int 
__init led_driver_init(void)
{
	int ret = 0;
	
	/* allocate device numbers dynamically */
	if(alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0)
	{
		printk(KERN_INFO "Can't register device\n");
		return -1;
	}

	dev_class = class_create(THIS_MODULE, DRIVER_NAME);
	
	/* allocate memory for the pulse_dev structure */
	dev_ptr = kmalloc(sizeof(struct led_dev), GFP_KERNEL);
	if(!dev_ptr)
	{
		printk(KERN_INFO "Bad kmalloc\n");
		return -ENOMEM;
	}

	strcpy(dev_ptr->name, DRIVER_NAME);

	/* connect file operations with cdev */
	cdev_init(&dev_ptr->cdev, &led_driver_fops);
	dev_ptr->cdev.owner = THIS_MODULE;

	/* connect major and minor numbers to cdev */
	ret = cdev_add(&dev_ptr->cdev, MKDEV(MAJOR(dev_num), MINOR(dev_num)), 1);
	if(ret)
	{
		printk("Bad cdev\n");
		return ret;
	}

	/* create device */
	device_create(dev_class, NULL, MKDEV(MAJOR(dev_num), MINOR(dev_num)), 
				  NULL, "%s", DEVICE_NAME);

	
	hrtimer_init(&my_hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_hrt.function = &timer_expiry_cb_fun;	
	
	printk("RGB-led module initialized\n");
	
	return 0;
}


/*************************************************************************
pulse_driver_exit

Description:
	1. Called when driver is removed from the kernel
	2. Frees device numbers and kernel memory

Return Value: void

*************************************************************************/
static void 
__exit led_driver_exit(void)
{
	/* free device number */
	unregister_chrdev_region(dev_num, 1);

	/* destroy device */
	device_destroy(dev_class, MKDEV(MAJOR(dev_num), 0));
	cdev_del(&dev_ptr->cdev);
	kfree(dev_ptr);

	/* destroy driver class */
	class_destroy(dev_class);
	hrtimer_cancel(&my_hrt);

	printk("RGB-led module removed\n");
}


module_init(led_driver_init);
module_exit(led_driver_exit);
