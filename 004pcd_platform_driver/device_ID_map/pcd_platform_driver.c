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
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include "platform.h"

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

struct plat_device_config pvdevConfig[] = {
    [PCDEVA1X] = {.config1 = 10, .config2 = 100},
    [PCDEVB1X] = {.config1 = 20, .config2 = 200},
    [PCDEVC1X] = {.config1 = 30, .config2 = 300},
};

/**
 * File Operations
 */

loff_t pcd_llseek (struct file *ptrFile, loff_t offset, int whence){
    struct pcdev_private_data *myDeviceData = (struct pcdev_private_data *)ptrFile->private_data;
    pr_info("llseek requested\n");
    pr_info("Current posotion of file pointer %lld\n",ptrFile->f_pos);
    loff_t tempOffset = 0;
    if(whence == SEEK_SET) {
        tempOffset = offset;
        if(tempOffset>myDeviceData->pDevData.size || tempOffset<0){
            return -EINVAL;
        }
    } else if(whence == SEEK_CUR) {
        tempOffset = ptrFile->f_pos + offset;
        if(tempOffset>myDeviceData->pDevData.size || tempOffset<0){
            return -EINVAL;
        }
    } else if (whence == SEEK_END) {
        tempOffset = myDeviceData->pDevData.size + offset;
        if(tempOffset>myDeviceData->pDevData.size || tempOffset<0){
            return -EINVAL;
        }
    } else {
        pr_info("Wrong entry for whence\n");
        return -EINVAL;
    }
    ptrFile->f_pos = tempOffset;
    pr_info("New value of the file position = %lld\n",ptrFile->f_pos);
    return 0;
}

ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos){
    struct pcdev_private_data *myDeviceData = (struct pcdev_private_data *)ptrFile->private_data;
    pr_info("Read request for %zu bytes for the device file %s\n", count,myDeviceData->pDevData.serialNumber);
    pr_info("Current position of file pointer %lld\n",*fPos);
    if((*fPos + count) > myDeviceData->pDevData.size){
        count = myDeviceData->pDevData.size - *fPos;
    }
    long bytesNotCopied = copy_to_user(buff, &myDeviceData->buffer[*fPos], count);
    if(bytesNotCopied){
        pr_info("Bad address, bytes not read:%zu",bytesNotCopied);
        return -EFAULT;
    }
    *fPos += count;
    pr_info("Number of bytes successfuly read %zu\n", count);
    pr_info("Updated file pointer position %lld\n",*fPos);
    return count;
}

ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos){
    struct pcdev_private_data *myDeviceData = ptrFile->private_data;
    pr_info("Write request for %zu bytes for the device file %s\n", count,myDeviceData->pDevData.serialNumber);
    pr_info("Current position of file pointer %lld\n",*fPos);
    if((*fPos + count) > myDeviceData->pDevData.size){
        count = myDeviceData->pDevData.size - *fPos;
    }
    if(!count){
        pr_err("No space left on pcd device\n");
        return -ENOMEM;
    }
    long bytesNotCopied = copy_from_user(&myDeviceData->buffer[*fPos], buff, count);
    if(bytesNotCopied){
        pr_info("Bad address, bytes not written:%zu",bytesNotCopied);
        return -EFAULT;
    }
    *fPos += count;
    pr_info("Number of bytes successfuly written %zu\n", count);
    pr_info("Updated file pointer position %lld\n",*fPos);
    return count;
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
    int ret;
    dev_t minorNum = MINOR(inode->i_rdev); //Fetching device minor number to show device in process
    pr_info("minor access = %d\n",minorNum);
    struct pcdev_private_data *myDeviceData = container_of(inode->i_cdev, struct pcdev_private_data, myCDev);
    ptrFile->private_data = myDeviceData;
    ret = check_permission(myDeviceData->pDevData.permissions, ptrFile->f_mode);
    if(ret == 0){
        pr_info("open was successful\n");
    } else {
        pr_info("open was unsuccessful\n");
    }
    return ret;
}

int pcd_release (struct inode *inode, struct file *ptrFile){
    pr_info("Close was successful\n");
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
        pr_info("Cannot Allocate Memory for Device Private Data\n");
        ret = -ENOMEM;
        goto out;
    }
    devData->pDevData.size = pData->size;
    devData->pDevData.permissions = pData->permissions;
    devData->pDevData.serialNumber = pData->serialNumber;
    pr_info("Device serial number = %s\n",devData->pDevData.serialNumber);
    pr_info("Device size = %d\n",devData->pDevData.size);
    pr_info("Device permissions = %d\n",devData->pDevData.permissions);

    pr_info("Patform driver config 1 = %d\n",pvdevConfig[pdev->id_entry->driver_data].config1);
    pr_info("Patform driver config 1 = %d\n",pvdevConfig[pdev->id_entry->driver_data].config2);

    //Dynamically allocate memory for the device buffer using size information from platform data
    devData->buffer = (char *)kzalloc(sizeof(char) * devData->pDevData.size, GFP_KERNEL);
    if(!devData->buffer){
        pr_info("Cannot Allocate Memory for Buffer\n");
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
    struct pcdev_private_data *devData = (struct pcdev_private_data*)dev_get_drvdata(&pdev->dev); //function returns pdev->dev.driver_data
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

struct platform_device_id pcdevIDs[] = {
    [0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
    //[PCDEVB1X] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X}, //Only IDs in this list can be added not everything mentioned in device_setup
    [1] = {.name = "pcdev-C1x", .driver_data = PCDEVC1X}
    /*When the probe function is called the driver_data index can be extracted and used to get configs for that device*/
};

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .id_table = pcdevIDs,
    .driver = {
        .name = "pseudo-char-device",//This is redundant now and not used for mapping
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