#include <stdio.h>
#include <fcntl.h>		/* open()  */
#include <unistd.h>		/* close() */
#include <stdlib.h>		/* exit()  */

#define LED "/dev/demo_dev"

int main(void)
{	
	int fd = 0, i = 0;

	fd = open(LED, O_WRONLY);
	if(fd == -1)
	{
		printf("Error opening file %s\n", LED);
		exit(-1);
	}	

	for(i = 0; i < 5; i++)
	{
		write(fd, NULL, 0);
	}

	close(fd);
	return 0;
}
