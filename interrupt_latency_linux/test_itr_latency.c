#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>	
#include <linux/delay.h>

#include "test_itr_latency.h"


/*
 *	struct itr_latency_data
 *
 *	@handler_time	: stores start time of ISR execution
 *	@trigger_time	: stores time right before square wave generation
 *	@irq 			: irq line number
 *	@missed_int		: counter for missed interrupts
 */
struct itr_latency_data
{
	struct timespec handler_time;
	struct timespec trigger_time;
	u16 irq;
	u8  missed_int;
}
*data_ptr;


/*
 *	interrupt handler
 *
 *	Details:
 *		- called when a rising edge at GPIO3 generates an interrupt signal
 *		- timestamps start time of handler execution
 *		- returns IRQ_HANDLED
 */
static irqreturn_t
interrupt_handler(int irq, void *dev_id)
{	
	static int count = 1;
	getnstimeofday(&data_ptr->handler_time);
	printk("\t interrupt handled, count: %d\n", count);
	count++;
	return IRQ_HANDLED;
}


/*
 *	measure_interrupt_latency
 *
 *	Details:
 *		- called by latency_module_init function
 *		- generates 'TOTAL_CYCLES' square wave cycles at GPIO2
 *		- timestamps time right before a high pulse is generated at GPIO2
 *		- calculates and prints interrupt latency to kernel log
 */
static void
measure_itr_latency(void)
{
	int i = 0;
	for(i = 0; i < TOTAL_CYCLES; i++)
	{
		printk(KERN_INFO "triggering cycle: %d\n", i + 1);
		getnstimeofday(&data_ptr->trigger_time);

		gpio_set_value(GPIO2, 1);
		udelay(50);
		gpio_set_value(GPIO2, 0);
		udelay(50);

		if(data_ptr->handler_time.tv_nsec == 0)
		{
			data_ptr->missed_int++;
			printk(KERN_INFO "\t interrupt missed, missed count: %d\n\n",
				data_ptr->missed_int);
		}
		else
		{
			printk(KERN_INFO "\t latency %lu ns\n\n",
				(data_ptr->handler_time.tv_nsec) - (data_ptr->trigger_time.tv_nsec));
			data_ptr->handler_time.tv_nsec = 0;
			data_ptr->trigger_time.tv_nsec = 0;
		}
	}
}


/*
 *	free_resources
 *
 *	Details:
 *		- called by latency_module_init function
 *		- frees gpio pins and irq line requested by demo_dev_open
 *		- returns 0 on success
 */
static void
free_resources(void)
{
	kfree(data_ptr);
	free_irq(data_ptr->irq, data_ptr);
	gpio_free(GPIO2);
	gpio_free(GPIO3);
	gpio_free(GPIO2_LEV);
	gpio_free(GPIO3_LEV);
}

/*
 *	latency_module_init
 *
 *	Details:
 *		- requests required gpio pins
 *		- requests irq line and registers interrupt_handler
 *		- calls helper functions
 *		- returns 0 on success, error otherwise
 */
static int 
__init latency_module_init(void)
{
	int ret = 0, irq_line = 0;

	/* allocate memory for the hcsr04_dev structure */
	data_ptr = kzalloc(sizeof(struct itr_latency_data), GFP_KERNEL);
	if(!data_ptr)
	{
		printk(KERN_INFO "bad kzalloc\n");
		return -ENOMEM;
	}

	gpio_free(GPIO2);
	gpio_free(GPIO3);
	gpio_free(GPIO2_LEV);
	gpio_free(GPIO3_LEV);

	gpio_request(GPIO2, "trigger");
	gpio_export(GPIO2,     false);
	gpio_export(GPIO2_LEV, false);	
	gpio_direction_output(GPIO2,     1);
	gpio_direction_output(GPIO2_LEV, 0);
	gpio_set_value(GPIO2, 0);

	gpio_request(GPIO3, "interrupt");
	gpio_export(GPIO3,     false);
	gpio_export(GPIO3_LEV, false);
	gpio_direction_input(GPIO3);
	gpio_direction_input(GPIO3_LEV);

	/* get irq line # from gpio */
	irq_line = gpio_to_irq(GPIO3);
	if(irq_line < 0)
	{
		printk(KERN_INFO "gpio%d cannot be used as interrupt", GPIO3);
		return EINVAL;
	}

	data_ptr->irq = irq_line;
	/* register interrupt handler function */
	ret = request_irq(irq_line,
			interrupt_handler,
			IRQF_TRIGGER_RISING,
			"irq_test_device",
			data_ptr);

	if(ret)
	{
		printk(KERN_INFO "unable to claim irq %d\n", irq_line);
		return ret;
	}

	printk(KERN_INFO "%s: module loaded\n", __FUNCTION__);

	measure_itr_latency();
	free_resources();
	
	return 0;
}
module_init(latency_module_init);


/*
 *	latency_module_exit
 *
 *	Details:
 *		- called when module unloaded from kernel
 */
static void 
__exit latency_module_exit(void)
{
	printk(KERN_INFO "%s: module unloaded\n", __FUNCTION__);
}
module_exit(latency_module_exit);


MODULE_AUTHOR("Sharvil Shah");
MODULE_LICENSE("GPL");
