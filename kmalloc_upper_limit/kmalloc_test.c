#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

/*
 *	kmalloc_test_init - init function of module
 *
 *	Details:
 *		- called when module loaded into kernel
 *		- makes repeated calls to kmalloc, to check the maximum
 *		  amount of RAM allocated with a single kmalloc call	 
 */
static int 
__init kmalloc_test_init(void)
{	
	size_t bytes = 1024;
	void * ptr = NULL;

	printk(KERN_INFO "%s: testing upper limit of kmalloc...\n", __FUNCTION__);
	printk(KERN_INFO "function  size(bytes) size(KiB) success\n");
	while(1)
	{	
		ptr = kmalloc(bytes, GFP_KERNEL);
		if(ptr)
		{
			printk(KERN_INFO "%s \t %8d %8d \t %s\n", 
					  "kmalloc", (int)bytes, (int)bytes/1024, "y");
			kfree(ptr);
		}
		else
		{
			printk(KERN_INFO "%s \t %8d %8d \t %s\n", 
					  "kmalloc", (int)bytes, (int)bytes/1024, "n");
			break;
		}

		bytes  *= 2;
	}

	return 0; 
}
module_init(kmalloc_test_init);


/*
 *	kmalloc_test_exit - exit function of module
 *
 *	Details:
 *		- called when module removed from kernel
 */
static void 
__exit kmalloc_test_exit(void)
{
	printk(KERN_INFO "%s: removing module\n", __FUNCTION__);
}
module_exit(kmalloc_test_exit);


MODULE_AUTHOR("Sharvil Shah");
MODULE_LICENSE("GPL v2");
