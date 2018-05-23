#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#define MAX 15	// max iterations of timer

static struct hrtimer my_hrt;	// global struct hrtimer
static ktime_t k_per_ns;

static enum hrtimer_restart timer_expiry_cb_fun(struct hrtimer *timer)
{	
	static volatile int itr = 0;

	ktime_t ktime_now;
	
	if(itr < MAX)
	{
		ktime_now = hrtimer_cb_get_time(timer);
		hrtimer_forward(timer, ktime_now, k_per_ns);
		printk(KERN_INFO "Timer iteration : %d Current ns: %lld\n", 
						  itr + 1, ktime_to_ns(ktime_now));
		itr++;
		return HRTIMER_RESTART;
	}
	else
	{
		return HRTIMER_NORESTART;
	}
}

static int __init my_hrtimer_init(void)
{	
	const unsigned long per_ns = 5E8L;	// timer period in nano secs

	printk(KERN_INFO "HZ: %d Jiffies: %llu Timer period: %lu ns\n",
					  HZ, get_jiffies_64(), per_ns);
	
	hrtimer_init(&my_hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_hrt.function = &timer_expiry_cb_fun;
		
	k_per_ns = ktime_set(0, per_ns);
	hrtimer_start(&my_hrt, k_per_ns, HRTIMER_MODE_REL);

	return 0;
}


static void __exit my_hrtimer_exit(void)
{	
	int ret = 0;
	while(hrtimer_callback_running(&my_hrt))
	{
		ret++;
	}

	if(ret != 0)
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

	printk(KERN_INFO "Exiting hrtimer\n");
}


module_init(my_hrtimer_init);
module_exit(my_hrtimer_exit);

MODULE_LICENSE("GPL v2");