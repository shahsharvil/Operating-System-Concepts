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
#include <linux/debugfs.h>

#include "debugfs_usage.h"

/*
 *	struct module_data
 *
 *	@dirret			: dentry object of our directory inside debugfs
 *	@intr_ret,@trig_ret	: dentry objects of children directories
 *	@irq 			: irq line number
 *	@interrupt_count	: counter for interrupts acknowledged
 *	@trigger_count		: counter for triggers (square wave cycles)
 *						generated at GPIO2
 */
struct module_data
{
	struct dentry *dirret;
	struct dentry *intr_ret;
	struct dentry *trig_ret;
	u16 irq;
	u64 interrupt_count;
	u64 trigger_count;
}
*data_ptr;


/*
 *	interrupt handler
 *
 *	Details:
 *		- called when a rising edge at GPIO3 generates an interrupt signal
 *		- acknowledges receipt of interrupt signal
 *		- returns IRQ_HANDLED
 */
static irqreturn_t
interrupt_handler(int irq, void *dev_id)
{	
	data_ptr->interrupt_count++;
	printk("\t interrupt handled..\n");
	return IRQ_HANDLED;
}


/*
 *	generate_square_waves
 *
 *	Details:
 *		- called by debugfs_usage_module_init function
 *		- generates 'TOTAL_CYCLES' square wave cycles at GPIO2
 */
static void
generate_square_waves(void)
{
	int i = 0;
	for(i = 0; i < TOTAL_CYCLES; i++)
	{
		printk(KERN_INFO "triggering cycle: %d\n", i + 1);

		gpio_set_value(GPIO2, 1);
		udelay(50);
		gpio_set_value(GPIO2, 0);
		udelay(50);
		data_ptr->trigger_count++;
	}
}


/*
 *	free_resources
 *
 *	Details:
 *		- called by debugfs_usage_module_exit function
 *		- frees gpio pins and irq line requested by debugfs_usage_module_init
 *		function
 *		- returns 0 on success
 */
static void
free_resources(void)
{	
	debugfs_remove_recursive(data_ptr->dirret);
	free_irq(data_ptr->irq, data_ptr);
	gpio_free(GPIO2);
	gpio_free(GPIO3);
	gpio_free(GPIO2_LEV);
	gpio_free(GPIO3_LEV);
	kfree(data_ptr);
}

/*
 *	debugfs_usage_module_init
 *
 *	Details:
 *		- requests required gpio pins
 *		- requests irq line and registers interrupt_handler
 *		- calls helper functions
 *		- returns 0 on success, error otherwise
 */
static int 
__init debugfs_usage_module_init(void)
{
	int ret = 0, irq_line = 0;

	/* allocate memory for the hcsr04_dev structure */
	data_ptr = kzalloc(sizeof(struct module_data), GFP_KERNEL);
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

	data_ptr->dirret = debugfs_create_dir("debugfs_usage_dir", NULL);
      
    	data_ptr->intr_ret = debugfs_create_u64("interrupt_count", 0444,
    		data_ptr->dirret, &data_ptr->interrupt_count);
    	if (!data_ptr->intr_ret)
    	{
        	printk("error creating int file");
        	return (-ENODEV);
    	}

   	data_ptr->trig_ret = debugfs_create_u64("trigger_count", 0444,
    		data_ptr->dirret, &data_ptr->trigger_count);
    	if (!data_ptr->trig_ret) {
        	printk("error creating int file");
        	return (-ENODEV);
    	}

	generate_square_waves();
	return 0;
}
module_init(debugfs_usage_module_init);


/*
 *	debugfs_usage_module_exit
 *
 *	Details:
 *		- called when module unloaded from kernel
 *		- frees resources previously requested
 */
static void 
__exit debugfs_usage_module_exit(void)
{	
	free_resources();
	printk(KERN_INFO "%s: module unloaded\n", __FUNCTION__);
}
module_exit(debugfs_usage_module_exit);


MODULE_AUTHOR("Sharvil Shah");
MODULE_LICENSE("GPL v2");
