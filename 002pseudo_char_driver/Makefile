obj-m += pcd.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
HOST_KERNEL_DIR=/lib/modules/$(shell uname -r)/build
BBB_KERNEL_DIR=/home/adhipshukla/Documents/BBB_Linux/ldd/source/linux-master-6.5.0/
all:
	make -C $(HOST_KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(HOST_KERNEL_DIR) M=$(PWD) clean
BBB:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(BBB_KERNEL_DIR) M=$(PWD) modules
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(BBB_KERNEL_DIR) M=$(PWD) help
