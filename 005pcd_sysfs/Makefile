obj-m += pcd_plat_drv_sysfs.o
pcd_plat_drv_sysfs-objs += pcd_platform_driver_sysfs.o pcd_syscalls.o
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
copy-dtb:
	scp /home/adhipshukla/Documents/BBB_Linux/ldd/source/linux-master-6.5.0/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb debian@192.168.6.2:/home/debian/ldd
copy-driver:
		scp ./*.ko debian@192.168.6.2:/home/debian/ldd
