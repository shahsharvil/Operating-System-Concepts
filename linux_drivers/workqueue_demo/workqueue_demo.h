/*
 *  workqueue_demo.h - definitions for rgbled character module
 */

#ifndef _WORKQUEUE_DEMO_H_
#define _WORKQUEUE_DEMO_H_

#define DEVICE_NAME "demo_dev"
#define DRIVER_NAME "demo_dev_driver"

/*	12 and 28 correspond to linux pin_no
 *	and level shifter pin resp. for GPIO1
 */
#define GPIO1 12		
#define GPIO1_LEV 28

/*	13 and 34 correspond to linux pin_no
 *	and level shifter resp. for GPIO2
 */
#define GPIO2 13
#define GPIO2_LEV 34

/*	12 and 28 correspond to linux pin_no
 *	and level shifter resp. for GPIO5
 */
#define led 0
#define led_ls 18

#endif /* _WORKQUEUE_DEMO_H_ */