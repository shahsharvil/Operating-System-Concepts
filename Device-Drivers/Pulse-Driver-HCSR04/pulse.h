#define DEVICE_NAME "pulse_device"
#define DRIVER_NAME "pulse_driver"

#define GP_IO2 13  		//GPIO13 corresponds to IO2
#define GP_IO3 14  		//GPIO14 corresponds to IO3
#define GP_IO2_LEV 34  		//GPIO34 corresponds to Level Shifter for IO2
#define GP_IO3_LEV 16  		//GPIO16 corresponds to Level Shifter for IO3

struct pulse_device_wrap
{
	struct cdev cdev;
	char name[25];
	int irq;
	unsigned int busy_flag;
	uint64_t pulse_rising_time;
	uint64_t pulse_falling_time;
}; 
