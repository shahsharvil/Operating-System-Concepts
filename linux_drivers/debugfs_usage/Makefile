obj-m := debugfs_usage.o

KDIR  := /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/src/kernel

all:	
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.ko *.o *.symvers *.order *.mod.*
