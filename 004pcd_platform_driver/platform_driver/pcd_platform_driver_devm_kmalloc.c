/**********************************************************************************************
* Header Section
***********************************************************************************************/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include "platform.h"
#include <linux/slab.h>

/**********************************************************************************************
* Code Section
***********************************************************************************************/
//Macros
#undef  pr_fmt
#define pr_fmt(fmt)         "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name
#define MAX_DEVICES         10

//Function decalration
loff_t pcd_llseek (struct file *ptrFile, loff_t off, int whence);
ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos);
ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos);
int pcd_open (struct inode *inode, struct file *ptrFile);
int pcd_release (struct inode *inode, struct file *ptrFile);
int check_permission(int devPerm, int accessMode);
int pcd_platform_driver_probe(struct platform_device *pdev);
int pcd_platform_driver_remove(struct platform_device *pdev);

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

struct pcdrv_private_data pcdrvData;

/**
 * File Operations
 */

loff_t pcd_llseek (struct file *ptrFile, loff_t offset, int whence){

    return 0;
}

ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos){
    
    return 0;
}

ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos){

    return -ENOMEM;
}

int check_permission(int devPerm, int accessMode){
    if(devPerm == RDWR)
        return 0;
    else if(devPerm == RDONLY && (accessMode & FMODE_READ) && !(accessMode & FMODE_WRITE))// using FMODE_READ/WRITE to check the accessmode requested
        return 0;
    else if (devPerm == WRONLY && !(accessMode & FMODE_READ) && (accessMode & FMODE_WRITE))
        return 0;
    else
        return -EPERM;
}

int pcd_open (struct inode *inode, struct file *ptrFile){

    return 0;
}

int pcd_release (struct inode *inode, struct file *ptrFile){
    
    return 0;
}

//File operations of the driver
struct file_operations pcd_fops = { .llseek = pcd_llseek,
                                    .read = pcd_read,
                                    .write = pcd_write,
                                    .open = pcd_open,
                                    .release = pcd_release,
                                    .owner = THIS_MODULE
                                  };

//Remove is called when matched platoform deivce is found
int pcd_platform_driver_probe(struct platform_device *pdev){
    int ret;
    struct pcdev_private_data *devData;
    struct pcdev_platform_data *pData;
    pr_info("A device is detected\n");

    //Get the platform data
    pData = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev); //Note: platform data is intialised while creating platform devices check pcd_platform_device.c
    if(!pData){
        pr_info("No platform data avaiable\n");
        ret = -EINVAL;
        goto out;
    }

    //Dynamically allocate memory for the device private data
    devData = (struct pcdev_private_data *)kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL); //Allocating memory dyncamically for device private data in kernel and zeroing out the allocated memory
    if(!devData){
        pr_info("Cannot Allocate Memory\n");
        ret = -ENOMEM;
        goto out;
    }
    devData->pDevData.size = pData->size;
    devData->pDevData.permissions = pData->permissions;
    devData->pDevData.serialNumber = pData->serialNumber;
    pr_info("Device serial number = %s\n",devData->pDevData.serialNumber);
    pr_info("Device size = %d\n",devData->pDevData.size);
    pr_info("Device permissions = %d\n",devData->pDevData.permissions);

    //Dynamically allocate memory for the device buffer using size information from platform data
    devData->buffer = (char *)kzalloc(sizeof(char) * devData->pDevData.size, GFP_KERNEL);
    if(!devData->buffer){
        pr_info("Cannot Allocate Memory\n");
        ret = -ENOMEM;
        goto dev_data_free;
    }

    //Set the device data in platform_device structure to access it in release function
    pdev->dev.driver_data = devData; //function dev_set_drvdata(&pdev->dev, devData); can also be used

    //Get the device number
    devData->devNum = pcdrvData.deviceNumBase + pdev->id; //id field is provided while creating platform devices check pcd_platform_device.c

    //cdev init and cdev add
    cdev_init(&devData->myCDev, &pcd_fops);
    devData->myCDev.owner = THIS_MODULE;
    ret = cdev_add(&devData->myCDev, devData->devNum, 1);
    if(ret<0){
        pr_err("Cdev add failure\n");
        goto buffer_free;
    }
    
    //created device file for the dected platform device
    pcdrvData.devicePCD = device_create(pcdrvData.classPCD, NULL, devData->devNum, NULL, "pcdev-%d",pdev->id);
    if(IS_ERR(pcdrvData.devicePCD)){
        pr_err("Device create failed\n");
        ret = PTR_ERR(pcdrvData.devicePCD);
        goto cdev_del;
    }
    pr_info("The probe was successful\n");
    pcdrvData.totalDevices++;//Inrease the number of platforms devices
    return 0;

    //error
cdev_del:
    device_destroy(pcdrvData.classPCD, devData->devNum);//Remove device from /dev/
    cdev_del(&devData->myCDev); //deleting cdev structure
buffer_free:
    kfree(devData->buffer);
dev_data_free:
    kfree(devData);
out: 
    pr_info("Device probe failed\n");
    return ret;
}

//Remove is called when the deivce is removed from the system
int pcd_platform_driver_remove(struct platform_device *pdev){
    //Get the device data
    struct pcdev_private_data *devData = (struct pcdev_private_data*)dev_get_drvdata(&pdev->dev); //fucntion returns pdev->dev.driver_data
    //Remove device that was created
    device_destroy(pcdrvData.classPCD, devData->devNum);
    //Remove a cdev entry from the system
    cdev_del(&devData->myCDev);
    //Free the memory help by the device
    kfree(devData->buffer);
    kfree(devData);
    pcdrvData.totalDevices--;//Decrement the total number of platform devices
    pr_info("A device is removed");
    return 0;
}

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver = {
        .name = "pseudo-char-device",
    },
};

static int __init pcd_driver_init(void){
    int ret;
    //Dynamically allocate a device number for a max number of devices
    ret = alloc_chrdev_region(&pcdrvData.deviceNumBase, 0, MAX_DEVICES, "pcdevs");
    if(ret<0){
        pr_err("Alloc chrdev failed\n");
        return ret;
    }
    //Create device class under /sys/class/
    pcdrvData.classPCD = class_create("pcd_class");
    if(IS_ERR(pcdrvData.classPCD)){
        pr_err("Class creation failed\n");
        ret = PTR_ERR(pcdrvData.classPCD);
        unregister_chrdev_region(pcdrvData.deviceNumBase, MAX_DEVICES);
        return ret;
    } 
    //Register platform driver
    ret = platform_driver_register(&pcd_platform_driver);
    if(ret<0){
        pr_err("Platform device register failed\n");
        return ret;
    }
    pr_info("pcd platform driver loaded");
    return 0;
}

static void __exit pcd_driver_cleanup(void){
    
    //Unregister the platform driver
    platform_driver_unregister(&pcd_platform_driver);

    //Class Destroy
    class_destroy(pcdrvData.classPCD);
    
    //Unregister device numbers for the max devices resitered
    unregister_chrdev_region(pcdrvData.deviceNumBase,MAX_DEVICES);    
    
    pr_info("pcd platform driver unloaded");
}

/**********************************************************************************************
* Resgistration Section
***********************************************************************************************/

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

/**********************************************************************************************
* Description Section
***********************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adhip");
MODULE_DESCRIPTION("Pseudo Character Device Driver");
MODULE_INFO(board, "BBB");