#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define PULSE_DEVICE_NAME "/dev/pulse_device"

int main(void)
{
	int i, fd, ret;
	float dis = 0;
	unsigned long long delta = 0;

	fd = open(PULSE_DEVICE_NAME, O_RDWR);
	if(fd == -1)
	{
		printf("Error opening file %s\n", PULSE_DEVICE_NAME);
 		exit(-1);
	}

	for(i = 1; i < 201; i++)
	{
		write(fd, NULL, 0);
		usleep(250000);

		ret = read(fd, &delta, 8);
		
		if(ret != -1)
		{	
			dis = ((float)delta*0.017) / 400;	
			printf("%5.2f cm\n", dis);
		}
		else
		{
			puts("Object out or range!");
		}
	}

	close(fd);

	return 0;
}
