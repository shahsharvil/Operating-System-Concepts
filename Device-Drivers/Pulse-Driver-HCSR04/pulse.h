/*
 * pulse.h - definitions for the char module
 */

#ifndef _PULSE_H_
#define _PULSE_H_

#define DEVICE_NAME "pulse_device"
#define DRIVER_NAME "pulse_driver"

#define GP_IO2 13  			/* GPIO13 corresponds to IO2 */
#define GP_IO3 14  			/* GPIO14 corresponds to IO3 */
#define GP_IO2_LEV 34  		/* GPIO34 corresponds to Level Shifter for IO2 */
#define GP_IO3_LEV 16  		/* GPIO16 corresponds to Level Shifter for IO3 */

struct pulse_dev
{
	struct cdev cdev;				/* char device structure*/
	char name[25];					
	int irq;						/* irq line number */
	unsigned int busy_flag;			/* used to synchronize pulse measurements */
	uint64_t pulse_rising_time;		/* time stamp of rising pulse */
	uint64_t pulse_falling_time;	/* time stamp of falling pulse */
}; 

#endif /* _PULSE_H_ *
