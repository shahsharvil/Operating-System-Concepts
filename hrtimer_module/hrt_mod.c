#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>

#define MAX 15

static struct hrtimer my_hrt;
static ktime_t period_ns;


/*
 *  timer_callback_func - callback function for hrtimer
 *
 *	Details:
 *		- called when timer expires the first time
 *		- extends expiry time of timer by "period_ns"
 *
 *	Return Value:
 *		- HRTIMER_RESTART or HRTIMER_NORESTART
 */
static enum hrtimer_restart timer_callback_func(struct hrtimer *timer)
{	
	static volatile int itr = 0;
	ktime_t ktime_now;

	if(itr < MAX)
	{
		ktime_now = hrtimer_cb_get_time(timer);
		hrtimer_forward(timer, ktime_now, period_ns);
		printk(KERN_INFO "%2d \t %llu \t %lld\n", 
						  itr + 1, get_jiffies_64(), ktime_to_ns(ktime_now));
		itr++;
		return HRTIMER_RESTART;
	}
	else
	{	
		return HRTIMER_NORESTART;
	}
}


/*
 *	hrt_mod_init - init function of module
 *
 *	Details:
 *		- called when module loaded into kernel
 *		- initializes and starts timer
 *
 * 	Note: The timer_callback_func is called when timer expires the first time		
 */
static int __init hrt_mod_init(void)
{	
	const unsigned long per_ns = 1E9L;

	printk(KERN_INFO "%s: HZ: %d, Timer period: %lu ns\n", __FUNCTION__, HZ, per_ns);
	printk(KERN_INFO "itr# \t jiffies \t uptime_ns");
	hrtimer_init(&my_hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_hrt.function = &timer_callback_func;
	period_ns = ktime_set(0, per_ns);
	hrtimer_start(&my_hrt, period_ns, HRTIMER_MODE_REL);

	return 0;
}
module_init(hrt_mod_init);


/*
 *	hrt_mod_init - exit function of module
 *
 *	Details:
 *		- called when module removed from kernel
 *		- waits for timer_callback_fun to complete whenever necessary
 *		- cancels hrtimer if timer is active/queued	
 */
static void __exit hrt_mod_exit(void)
{	
	int ret = 0;
	while(hrtimer_callback_running(&my_hrt))
	{
		ret++;
	}

	if(ret)
	{
		printk(KERN_INFO "Waited for hrtimer callback to complete (%d)\n", ret);
	}

	if(hrtimer_active(&my_hrt) != 0)
	{
		ret = hrtimer_cancel(&my_hrt);
		printk(KERN_INFO "hrtimer cancelled (%d)\n", ret);
	}

	if(hrtimer_is_queued(&my_hrt) != 0)
	{
		ret = hrtimer_cancel(&my_hrt);
		printk(KERN_INFO "hrtimer cancelled (%d)\n", ret);
	}

	printk(KERN_INFO "%s: Exiting hrtimer\n", __FUNCTION__);
}
module_exit(hrt_mod_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sharvil Shah, spshah16@asu.edu");