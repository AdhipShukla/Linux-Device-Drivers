#include"pcd_platform_driver_sysfs.h"

//Function decalration
int pcd_platform_driver_probe(struct platform_device *pdev);
int pcd_platform_driver_remove(struct platform_device *pdev);
struct pcdev_platform_data * get_plat_data_from_device_tree(struct device *dev);
ssize_t (max_size_show)(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t (max_size_store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t (serial_num_show)(struct device *dev, struct device_attribute *attr, char *buf);
int pcd_sysfs_create_files(struct device *pcdDev);

//Driver private data
struct pcdrv_private_data pcdrvData;

//Platform device configuration data
struct plat_device_config pvdevConfig[] = {
    [PCDEVA1X] = {.config1 = 10, .config2 = 100},
    [PCDEVB1X] = {.config1 = 20, .config2 = 200},
    [PCDEVC1X] = {.config1 = 30, .config2 = 300},
};

struct of_device_id pcdevDeviceTreeData[];

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

ssize_t max_size_show(struct device *dev, struct device_attribute *attr, char *buf){
    //Get access to the device priate data
    struct pcdev_private_data *devData = dev_get_drvdata(dev->parent);
    return  sprintf(buf,"%d\n",devData->pDevData.size);//sprintf writtens the number of character written to buffer
}

ssize_t max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    long result;
    int ret;
    struct pcdev_private_data *devData = dev_get_drvdata(dev->parent);
    ret = kstrtol(buf,0,&result);//This function converts strings to long interger, the second argument decides the base of int 0 means automatic detection of decimal, hex, oct
    if(ret){
        return ret;
    }
    devData->pDevData.size = result;
    devData->buffer = krealloc(devData->buffer, devData->pDevData.size, GFP_KERNEL);//Using this user can redefine the size of buffer set from device setup module or device tree
    return count;
}

ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf){ //The first argument is a refeence to the device that was created when using device_create API but we need the parent of this device which was instantiated from device tree to get device data
    //Get access to the device priate data
    struct pcdev_private_data *devData = dev_get_drvdata(dev->parent);
    return  sprintf(buf,"%s\n",devData->pDevData.serialNumber);//sprintf writtens the number of character written to buffer
}

//Defining variables of struct device attributes
static DEVICE_ATTR(max_size,S_IRUGO|S_IWUSR,max_size_show,max_size_store); //This macro is defined in device.h and is used to create device attributes files, specify permissions and read/write methods.
static DEVICE_ATTR(serial_num,S_IRUGO,serial_num_show,NULL); //S_IRUGO defined read permissions and S_IWUSR defines write permissions

struct attribute *pcdAttrs[] = {//Defining an array of attributes
    &dev_attr_max_size.attr,
    &dev_attr_serial_num.attr,
    NULL
};

struct attribute_group pcdAttrGrp = {
    .attrs = pcdAttrs,
};
//Helper function to create attributes
int pcd_sysfs_create_files(struct device *pcdDev){
    /*int ret;
    ret = sysfs_create_file(&pcdDev->kobj,&dev_attr_max_size.attr); //This API to define the attribute individually
    if(ret){
        return ret;
    }
    ret = sysfs_create_file(&pcdDev->kobj,&dev_attr_serial_num.attr);
    return ret;*/
    return sysfs_create_group(&pcdDev->kobj,&pcdAttrGrp);
}

//File operations of the driver
struct file_operations pcd_fops = { .llseek = pcd_llseek,
                                    .read = pcd_read,
                                    .write = pcd_write,
                                    .open = pcd_open,
                                    .release = pcd_release,
                                    .owner = THIS_MODULE
                                  };

//Probe is called when matched platoform deivce is found
int pcd_platform_driver_probe(struct platform_device *pdev){
    int ret;
    struct device *dev = &pdev->dev; //Device that is instantiated by adding device kernel module or device tree
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
    dev_info(dev,"Patform driver config 2 = %d\n",pvdevConfig[driverData].config2);

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
        pcdrvData.devicePCD = device_create(pcdrvData.classPCD, dev, devData->devNum, NULL, "pcdev-%d",pdev->id);
    else
        pcdrvData.devicePCD = device_create(pcdrvData.classPCD, dev, devData->devNum, NULL, "pcdev-%d",pcdrvData.totalDevices);
    
    if(IS_ERR(pcdrvData.devicePCD)){
        dev_err(dev,"Device create failed\n");
        ret = PTR_ERR(pcdrvData.devicePCD);
        goto cdev_del;
    }

    ret = pcd_sysfs_create_files(pcdrvData.devicePCD);
    if(ret){
        dev_err(dev,"Attributes can't be created\n");
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