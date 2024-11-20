/**********************************************************************************************
* Header Section
***********************************************************************************************/
#ifndef PCD_PLATFORM_DT_SYSFS_H
#define PCD_PLATFORM_DT_SYSFS_H 

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "platform.h"

/**********************************************************************************************
* Typedefs
***********************************************************************************************/

//Macros
#undef  pr_fmt
#define pr_fmt(fmt)         "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name
#define MAX_DEVICES         10

//Device private data structure
struct pcdev_private_data{
    struct pcdev_platform_data pDevData;
    char *buffer;
    dev_t devNum;
    struct cdev myCDev;
};

//Driver private data structure
struct pcdrv_private_data{
    int totalDevices;
    dev_t deviceNumBase;
    struct class *classPCD;
    struct device *devicePCD;
};

//Driver config items
struct plat_device_config{
    int config1;
    int config2;
};

enum pcdev_names {
    PCDEVA1X,
    PCDEVB1X,
    PCDEVC1X,
    PCDEVCOUNT
};

/**********************************************************************************************
* Public Functions
***********************************************************************************************/

//Function decalration
loff_t pcd_llseek (struct file *ptrFile, loff_t off, int whence);
ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos);
ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos);
int pcd_open (struct inode *inode, struct file *ptrFile);
int pcd_release (struct inode *inode, struct file *ptrFile);
int check_permission(int devPerm, int accessMode);

#endif//PCD_PLATFORM_DT_SYSFS_H