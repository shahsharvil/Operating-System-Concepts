/*
 * rgbled.h - definitions for the char led module
 */

#ifndef _RGBLED_H_
#define _RGBLED_H_

#define DEVICE_NAME "RGB_Led"
#define DRIVER_NAME "RGB_Led_driver"

#define per_ns 10000000		/* length of 1 pwm period */
#define MAX 200				/* total timer iterations */

int linux_io[6] = { 11, 12, 13, 14,  6,  0 };	/* linux pin mappings for Galileo Gen 2*/
int level_sh[6] = { 32, 28, 34, 16, 36, 18 };   /* level shifter mappings */

struct led_dev
{
	struct cdev cdev;				/* char device structure*/
	char name[25];					/* driver name */
	int red, green, blue;			/* board pin numbers */
	int red_ls, green_ls, blue_ls;	/* corresponding level shifters */
	int pwm;						/* pwm intensity */
	unsigned long on_time_ns;		/* on time in ns per pwm period */
	unsigned long off_time_ns;      /* off time in ns per pwm period */
};

struct ioctl_message						
{
	int r_pin;
	int g_pin;
	int b_pin;
    int pwm;						/* duty cycle value */
};

#endif /* _RGBLED_H_ */
