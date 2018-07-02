TOOLDIR = /opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

obj-m := workqueue_demo.o

KDIR  := /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/src/kernel

CC=$(TOOLDIR)/i586-poky-linux-gcc

all:	
	make -C $(KDIR) M=$(PWD) modules
	$(CC) -Wall -o app app.c 

clean:
	rm -f app *.ko *.o *.symvers *.order *.mod.*
