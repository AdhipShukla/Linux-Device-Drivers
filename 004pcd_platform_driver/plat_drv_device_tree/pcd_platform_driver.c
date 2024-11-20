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
#include <linux/of.h>
#include <linux/of_device.h>
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
struct pcdev_platform_data * get_plat_data_from_device_tree(struct device *dev);

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

struct of_device_id pcdevDeviceTreeData[];

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
        pr_info("Bad address, bytes not read:%lu",bytesNotCopied);
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
        pr_info("Bad address, bytes not written:%lu",bytesNotCopied);
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

struct pcdev_platform_data * get_plat_data_from_device_tree(struct device *dev){
    
    struct device_node *devNode = dev->of_node; //This field in the device structure is only intialized when device tree is used for mapping
    struct pcdev_platform_data *pData;
    
    if(!devNode){//The probe was not successful because of device tree and could be because ID tabel or name mapping
        return NULL;
    }

    pData = devm_kzalloc(dev, sizeof(struct pcdev_platform_data), GFP_KERNEL); //Dynamic allocation maintained by the device
    if(!pData){
        dev_info(dev,"Cannot allocate memory\n");
        return ERR_PTR(-ENOMEM);
    }

    //of_property_read_string is a kernel API that iterates over all the properties save as linied list inside device node to find the property passed
    if(of_property_read_string(devNode,"personal,device-serial-num",&pData->serialNumber)){
        dev_info(dev,"Missing serial number property");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(devNode,"personal,size",&pData->size)){
        dev_info(dev,"Missing size property");
        return ERR_PTR(-EINVAL);
    }
    
    if(of_property_read_u32(devNode,"personal,permission",&pData->permissions)){
        dev_info(dev,"Missing permissions property");
        return ERR_PTR(-EINVAL);
    }

    return pData;
}

//Remove is called when matched platoform deivce is found
int pcd_platform_driver_probe(struct platform_device *pdev){
    int ret;
    struct device *dev = &pdev->dev;
    struct pcdev_private_data *devData;
    struct pcdev_platform_data *pData;
    struct of_device_id const *match;
    struct of_device_id *openFirmwareFlag = of_match_ptr(pcdevDeviceTreeData); //If this is null that means match table was not initialized
    int driverData;

    dev_info(dev,"A device is detected\n");
    
    pData = get_plat_data_from_device_tree(dev);
    
    if(IS_ERR(pData)){
        return PTR_ERR(pData);
    }

    if(!pData || !openFirmwareFlag){
        //Get the platform data
        dev_info(dev,"Using the device ID based mapping");
        pData = (struct pcdev_platform_data*)dev_get_platdata(dev); //Note: platform data is intialised while creating platform devices check pcd_platform_device.c
        if(!pData){
            dev_info(dev,"No platform data avaiable\n");
            ret = -EINVAL;
            goto out;
        }
        driverData = pdev->id_entry->driver_data; //Can only be used when using ID table for mapping
    } else {
        //The next two lines could be repalced by kernel API of_device_get_match_data(dev)
        match = of_match_device(pdev->dev.driver->of_match_table, dev);//This is extracing the data provided while registering driver by matching compatible property of current device with the "pcdevDeviceTreeData" array that was passed
        driverData = (int)match->data;
    }
    //Dynamically allocate memory for the device private data
    devData = (struct pcdev_private_data *)kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL); //Allocating memory dyncamically for device private data in kernel and zeroing out the allocated memory
    if(!devData){
        dev_info(dev,"Cannot Allocate Memory for Device Private Data\n");
        ret = -ENOMEM;
        goto out;
    }
    devData->pDevData.size = pData->size;
    devData->pDevData.permissions = pData->permissions;
    devData->pDevData.serialNumber = pData->serialNumber;
    dev_info(dev,"Device serial number = %s\n",devData->pDevData.serialNumber);
    dev_info(dev,"Device size = %d\n",devData->pDevData.size);
    dev_info(dev,"Device permissions = %d\n",devData->pDevData.permissions);

    dev_info(dev,"Patform driver config 1 = %d\n",pvdevConfig[driverData].config1);
    dev_info(dev,"Patform driver config 1 = %d\n",pvdevConfig[driverData].config2);

    //Dynamically allocate memory for the device buffer using size information from platform data
    devData->buffer = (char *)kzalloc(sizeof(char) * devData->pDevData.size, GFP_KERNEL);
    if(!devData->buffer){
        dev_info(dev,"Cannot Allocate Memory for Buffer\n");
        ret = -ENOMEM;
        goto dev_data_free;
    }

    //Set the device data in platform_device structure to access it in release function
    pdev->dev.driver_data = devData; //function dev_set_drvdata(dev, devData); can also be used

    //Get the device number
    if(!pData) //If device tree based mapping then we can't use pdev->id
        devData->devNum = pcdrvData.deviceNumBase + pdev->id; //id field is provided while creating platform devices check pcd_platform_device.c
    else
        devData->devNum = pcdrvData.deviceNumBase + pcdrvData.totalDevices;

    //cdev init and cdev add
    cdev_init(&devData->myCDev, &pcd_fops);
    devData->myCDev.owner = THIS_MODULE;
    ret = cdev_add(&devData->myCDev, devData->devNum, 1);
    if(ret<0){
        dev_err(dev,"Cdev add failure\n");
        goto buffer_free;
    }
    
    //created device file for the dected platform device
    if(!pData) //If device tree based mapping then we can't use pdev->id
        pcdrvData.devicePCD = device_create(pcdrvData.classPCD, NULL, devData->devNum, NULL, "pcdev-%d",pdev->id);
    else
        pcdrvData.devicePCD = device_create(pcdrvData.classPCD, NULL, devData->devNum, NULL, "pcdev-%d",pcdrvData.totalDevices);
    
    if(IS_ERR(pcdrvData.devicePCD)){
        dev_err(dev,"Device create failed\n");
        ret = PTR_ERR(pcdrvData.devicePCD);
        goto cdev_del;
    }
    dev_info(dev,"The probe was successful\n");
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
    dev_info(dev,"Device probe failed\n");
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
    dev_info(&pdev->dev,"A device is removed");
    return 0;
}

// This is relevant for ID table based mapping
struct platform_device_id pcdevIDs[] = {
    [0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
    [1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X}, //Only IDs in this list can be added not everything mentioned in device_setup
    [2] = {.name = "pcdev-C1x", .driver_data = PCDEVC1X},
    {}
    /*When the probe function is called the driver_data index can be extracted and used to get configs for that device*/
};

// This is relevant for device tree based mapping
struct of_device_id pcdevDeviceTreeData[] = { //The device tree based mapping will take presedence over id based and name based mapping
    [0] = {.compatible = "pcdev-A1x", .data = (void *)PCDEVA1X},
    [1] = {.compatible = "pcdev-B1x", .data = (void *)PCDEVB1X},
    [2] = {.compatible = "pcdev-C1x", .data = (void *)PCDEVC1X},
    {}   //Null termination to let the process know end of array
};

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .id_table = pcdevIDs,//This passes all the supported devices by this driver
    .driver = {
        .name = "pseudo-char-device",//This is redundant now and not used for mapping
        .of_match_table = of_match_ptr(pcdevDeviceTreeData)//Driver data could include configuration and other things
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


debian@BeagleBone:/sys/class/pcd_class/pcdev-0$ ls
dev  power  subsystem  uevent
