/*************************************************************************

File Name: pulse.c

Description: A character device driver for pulse measurements of
			 an Ultrasonic Distance Sensor (HCSR04).

Author: Sharvil Shah (spshah16@asu.edu)

*************************************************************************/


#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/slab.h>

// custom header files
#include "pulse.h"
#include "rdtsc.h"

static dev_t pulse_device_num;
struct class *device_class;
struct pulse_device_wrap *pulse_device_ptr;

bool echo_pulse_rec = false;


/*************************************************************************
Interrupt Handler: pulse_interrupt_handler

Description:
		
	1. Called when a rising or falling edge generates an interrupt signal
	2. Timestamps interrupt arrival and toggles type between 
	   IRQF_TRIGGER_FALLING and IRQF_TRIGGER_RISING	
	3. Sets busy_flag to 0 after stamping both rising and falling edges
	   of the echo signal

Return Value: IRQ_HANDLED

*************************************************************************/
static irqreturn_t
pulse_interrupt_handler(int irq, void *dev_id)
{	
	if(echo_pulse_rec == false)
	{
		pulse_device_ptr->pulse_rising_time = tsc();
	    irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
	    echo_pulse_rec = true;
	}
	else
	{
		pulse_device_ptr->pulse_falling_time = tsc();
	    irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	    echo_pulse_rec = false;
		pulse_device_ptr->busy_flag = 0;
	}
	return IRQ_HANDLED;
}


/*************************************************************************
pulse_driver_open

Input Arguments:
	1. inode - pointer to inode structure
	2. filp  - pointer to file

Description:
	1. Called when device opened.
	2. Requests gpio pins needed for functioning of the device.
	3. Registers the interrupt handler to handle echo signals

Return Value: 0 on success 	

*************************************************************************/
int
pulse_driver_open(struct inode *inode, struct file *filp)
{	
	int ret = 0, irq_line_no = 0;

	printk(KERN_INFO "Driver: open()\n");

	pulse_device_ptr = container_of(inode->i_cdev, struct pulse_device_wrap, cdev);
	filp->private_data = pulse_device_ptr;

	// free and request gpio's to work with
	gpio_free(GP_IO2);
	gpio_free(GP_IO3);
	gpio_free(GP_IO2_LEV);
	gpio_free(GP_IO3_LEV);

	gpio_request(GP_IO2, "Trigger");
	gpio_export(GP_IO2, false);
	gpio_export(GP_IO2_LEV, false);	
	gpio_direction_output(GP_IO2, 1);
	gpio_direction_output(GP_IO2_LEV, 0);
	
	gpio_request(GP_IO3, "Echo");	
	gpio_export(GP_IO3, false);
	gpio_export(GP_IO3_LEV, false);	
	gpio_direction_input(GP_IO3);
	gpio_direction_input(GP_IO3_LEV);
	
	// get irq # from gpio
	irq_line_no = gpio_to_irq(GP_IO3);
	if(irq_line_no < 0)
	{
		printk("gpio%d cannot be used as interrupt", GP_IO3);
		return -EINVAL;
	}
	
	pulse_device_ptr->irq = irq_line_no;	
	pulse_device_ptr->pulse_rising_time = 0;
	pulse_device_ptr->pulse_falling_time = 0;
	
	// register interrupt handler function
	ret = request_irq(irq_line_no,
					  (irq_handler_t)pulse_interrupt_handler,
					  IRQF_TRIGGER_RISING,
					  "interrupt_state_change", 
					  pulse_device_ptr);

	if(ret)
	{
		printk("Unable to claim irq %d; error %d\n ", irq_line_no, ret);
		return -1;
	}
	
	return 0;
}



/*************************************************************************
pulse_driver_release

Input Arguments:
	1. inode - pointer to inode structure
	2. filp  - pointer to file

Description:
	1. Called when device released.
	2. Frees the irq and gpio pins previously requested.

Return Value: 0 on success 

*************************************************************************/
int
pulse_driver_release(struct inode *inode, struct file *filp)
{
	struct pulse_device_wrap *local_device_ptr;
	
	pulse_device_ptr->busy_flag = 0;
	local_device_ptr = filp->private_data;

	// free irq and gpio
	free_irq(pulse_device_ptr->irq, pulse_device_ptr);
	
	gpio_free(GP_IO2);
	gpio_free(GP_IO3);
	gpio_free(GP_IO2_LEV);
	gpio_free(GP_IO3_LEV);
	
	printk("Driver: close()\n");
	return 0;
}


/*************************************************************************
pulse_driver_read

Input Arguments:
	1. file  - pointer to file
	2. buf   - address of user space buffer
	3. count - size of buffer
	4. prt   - position offset

Description:
	1. Computes and copies difference of falling and rising
	   time stamps from driver code to user space

	(Note: This function can be modified to return the actual distance
	       instead of difference of time stamps)   

Return Value: 0 on success 

*************************************************************************/
static ssize_t
pulse_driver_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	int ret = 0;
	uint64_t delta;

	// return error if time delta not ready
	if(pulse_device_ptr->busy_flag == 1)
	{
		return -EBUSY;
	}
	else
	{
		if(pulse_device_ptr->pulse_rising_time == 0 && 
			pulse_device_ptr->pulse_falling_time == 0)
		{
			printk("Please Trigger the measure first\n");
		}
		else
		{	
			// copy latest time delta computed to user space
			delta = (pulse_device_ptr->pulse_falling_time) - 
				    (pulse_device_ptr->pulse_rising_time);			

			ret = copy_to_user((void *)buf, (const void *)&delta, sizeof(delta));

			pulse_device_ptr->pulse_falling_time = 0; 
			pulse_device_ptr->pulse_rising_time = 0;	
		}
	}

	return ret;
}


/*************************************************************************
pulse_driver_write

Input Arguments:
	1. file  - pointer to file
	2. buf   - address of user space buffer
	3. count - size of buffer
	4. ppos   - position offset

Description:
	1. Generates a trigger pulse of 15 us duration on the trigger pin
	2. Sets busy_flag to 1.

Return Value: 0 on success 

*************************************************************************/
static ssize_t
pulse_driver_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{	
	if(pulse_device_ptr->busy_flag == 1)
	{
		return -EBUSY;
	}	
	
	// generate a trigger pulse
	gpio_set_value(GP_IO2, 1);
	udelay(15);
	gpio_set_value(GP_IO2, 0);

	pulse_device_ptr->busy_flag = 1;
	return 0;
}


/*
 * File operations structure for the device
 */
static struct file_operations pulse_driver_fops = 
{
	.owner   = THIS_MODULE,
	.open    = pulse_driver_open,
	.release = pulse_driver_release,
	.read    = pulse_driver_read,
	.write   = pulse_driver_write
};


/*************************************************************************
pulse_driver_init

Description:
	1. Called when driver loaded into the kernel
	2. Initializes the Ultrasonic Distance Sensor

Return Value: 0 on success

*************************************************************************/
static int 
__init pulse_driver_init(void)
{
	int ret;

	// allocate device numbers dynamically
	if(alloc_chrdev_region(&pulse_device_num, 0, 1, DEVICE_NAME) < 0)
	{
		printk(KERN_INFO "Can't register device\n");
		return -1;
	}

	device_class = class_create(THIS_MODULE, DRIVER_NAME);
	
	// allocate memory for the pulse_device_wrap structure
	pulse_device_ptr = kmalloc(sizeof(struct pulse_device_wrap), GFP_KERNEL);
	if(!pulse_device_ptr)
	{
		printk(KERN_INFO "Bad kmalloc\n");
		return -ENOMEM;
	}

	strcpy(pulse_device_ptr->name, DRIVER_NAME);

	// connect file operations with cdev
	cdev_init(&pulse_device_ptr->cdev, &pulse_driver_fops);
	pulse_device_ptr->cdev.owner = THIS_MODULE;

	// connect major and minor numbers to cdev
	ret = cdev_add(&pulse_device_ptr->cdev, 
		           MKDEV(MAJOR(pulse_device_num), MINOR(pulse_device_num)), 1);
	if(ret)
	{
		printk("Bad cdev\n");
		return ret;
	}

	// create the device
	device_create(device_class, NULL, 
		          MKDEV(MAJOR(pulse_device_num), MINOR(pulse_device_num)), 
				  NULL, "%s", DEVICE_NAME);

	printk("Pulse driver Initialized.\n");
	
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
__exit pulse_driver_exit(void)
{
	// release major number
	unregister_chrdev_region(pulse_device_num, 1);

	// destroy device with Minor number 0
	device_destroy(device_class, MKDEV(MAJOR(pulse_device_num), 0));
	cdev_del(&pulse_device_ptr->cdev);
	kfree(pulse_device_ptr);

	// destroy driver class
	class_destroy(device_class);
	printk("Module removed\n");
}


module_init(pulse_driver_init);
module_exit(pulse_driver_exit);

MODULE_AUTHOR("Sharvil Shah");
MODULE_DESCRIPTION("Pulse Driver");
MODULE_LICENSE("GPL v2");