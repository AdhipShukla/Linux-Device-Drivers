/**********************************************************************************************
* Header Section
***********************************************************************************************/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

/**********************************************************************************************
* Code Section
***********************************************************************************************/
//Macros
#define DEV_MEM_SIZE_PCDEV1 512
#define DEV_MEM_SIZE_PCDEV2 256
#define DEV_MEM_SIZE_PCDEV3 1024
#define DEV_MEM_SIZE_PCDEV4 128
#undef  pr_fmt
#define pr_fmt(fmt)         "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name
#define RDONLY              0X01
#define WRONLY              0X10
#define RDWR                0X11

//Enums
typedef enum {
    DEV_MEM_PCDEV1 = 0,
    DEV_MEM_PCDEV2,
    DEV_MEM_PCDEV3,
    DEV_MEM_PCDEV4,
    NO_OF_DEVICES
} pcdDevices;

//Pseudo Device Memory
char device_buffer_pcdev1[DEV_MEM_SIZE_PCDEV1];
char device_buffer_pcdev2[DEV_MEM_SIZE_PCDEV2];
char device_buffer_pcdev3[DEV_MEM_SIZE_PCDEV3];
char device_buffer_pcdev4[DEV_MEM_SIZE_PCDEV4];

//Device specific private data struct
struct pcdev_private_data{
    char *buffer;
    unsigned size;
    const char *serialNumber;
    int permissions;
    struct cdev myCdev;
    struct device *devicePCD;
};

//Driver private data struct
struct pcdrv_private_data{
    int totalDevices;
    struct pcdev_private_data pcdevData[NO_OF_DEVICES];
    dev_t deviceNumber;
    struct class *classPCD;
};

struct pcdrv_private_data pcdrvData = {
    .totalDevices = NO_OF_DEVICES,
    .pcdevData = {
            [DEV_MEM_PCDEV1] = {
                .buffer         = device_buffer_pcdev1,
                .size           = DEV_MEM_SIZE_PCDEV1,
                .serialNumber  = "PCDEV0",
                .permissions    = RDONLY //Read Only
            },
            [DEV_MEM_PCDEV2] = {
                .buffer         = device_buffer_pcdev2,
                .size           = DEV_MEM_SIZE_PCDEV2,
                .serialNumber  = "PCDEV1",
                .permissions    = WRONLY //Write Only
            },
            [DEV_MEM_PCDEV3] = {
                .buffer         = device_buffer_pcdev3,
                .size           = DEV_MEM_SIZE_PCDEV3,
                .serialNumber  = "PCDEV2",
                .permissions    = RDWR //Read and Write Only
            },
            [DEV_MEM_PCDEV4] = {
                .buffer         = device_buffer_pcdev4,
                .size           = DEV_MEM_SIZE_PCDEV4,
                .serialNumber  = "PCDEV3",
                .permissions    = RDWR //Read and Write Only
            }  
    }
};

//Function decalration
loff_t pcd_llseek (struct file *ptrFile, loff_t off, int whence);
ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos);
ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos);
int pcd_open (struct inode *inode, struct file *ptrFile);
int pcd_release (struct inode *inode, struct file *ptrFile);
int check_permission(int devPerm, int accessMode);

/**
 * File Operations
 */

loff_t pcd_llseek (struct file *ptrFile, loff_t offset, int whence){

    struct pcdev_private_data *pcdevData = (struct pcdev_private_data *)ptrFile->private_data;
    pr_info("llseek requested\n");
    loff_t tempOffset = 0;
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    if(whence == SEEK_SET){
        tempOffset = offset;
        if(tempOffset > pcdevData->size || tempOffset < 0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else if (whence == SEEK_CUR) {
        tempOffset = ptrFile->f_pos + offset;
        if(tempOffset > pcdevData->size || tempOffset < 0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else if (whence == SEEK_END) {
        tempOffset = pcdevData->size + offset;
        if(tempOffset > pcdevData->size || tempOffset < 0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else {
        return -EINVAL; //Invalid value passed for whence
    }

    return ptrFile->f_pos;
}

ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos){
    
    struct pcdev_private_data *pcdevData = (struct pcdev_private_data *)ptrFile->private_data;
    pr_info("read requested for %zu bytes\n", count);
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    /*Adjusting the number of bytes to be read*/
    if(*fPos + count > pcdevData->size){
        count = pcdevData->size - *fPos;
    }
    /*Device 0 when called fetches value from device 1 which is write only*/
    if(strcmp(pcdevData->serialNumber, "PCDEV0") == 0){
        pr_info("Copying %zu bytes from device one\n",count);
        int bytes = strlen(pcdrvData.pcdevData[1].buffer);
        if(bytes > pcdevData->size){
            bytes = pcdevData->size;
        }
        memcpy(pcdevData->buffer,pcdrvData.pcdevData[1].buffer,bytes);
    }

    /*Copying the byte to user address from the pseudo device*/
    long bytesNotCopied = copy_to_user(buff, &pcdevData->buffer[*fPos], count);
    if(bytesNotCopied){
        pr_info("Bad address bytes left to read %ld", bytesNotCopied);
        return -EFAULT;
    }
    /*Updaing the file position*/
    *fPos += count;
    pr_info("Number of bytes successfully read = %zu\n", count);
    pr_info("Updated file position %lld\n", *fPos);
    /*Returing the number of bytes successully copied*/
    return count;
}

ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos){

    struct pcdev_private_data *pcdevData = (struct pcdev_private_data *)ptrFile->private_data;
    pr_info("write requested for %zu bytes\n", count);
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    /*Adjusting the number of bytes to be written*/
    if(*fPos + count > pcdevData->size){
        count = pcdevData->size - *fPos;
    }
    /*If Count is zero*/
    if(!count){
        pr_err("No space left on pcd device\n");
        return -ENOMEM;
    }
    /*Copying the byte from user address to pseudo device*/
    long bytesNotCopied = copy_from_user(&pcdevData->buffer[*fPos], buff, count);
    if(bytesNotCopied){
        pr_info("Bad address bytes left to wirtten %ld", bytesNotCopied);
        return -EFAULT;
    }
    /*Updaing the file position*/
    *fPos += count;

    pr_info("Number of bytes successfully written = %zu\n", count);
    pr_info("Updated file position %lld\n", *fPos);

    /*Returing the number of bytes successully copied*/
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
    int minorNum;
    struct pcdev_private_data *pcdevData;
    //Find out the specific device file for which the open was called
    minorNum = MINOR(inode->i_rdev); //Minor macro(extracts last 20 bits) takes out the device specific number ///ly MAJOR extracts the driver number
    pr_info("minor access = %d\n",minorNum);
    //Fetch device's private data structure
    pcdevData = container_of(inode->i_cdev, struct pcdev_private_data, myCdev); //Using container_of macro to get parent struct
    //Add the device private data structure to file structure's private data that can be used by read, write and other functions  
    ptrFile->private_data = pcdevData;
    //check permission
    ret = check_permission(pcdevData->permissions, ptrFile->f_mode);
    if(ret == 0){
        pr_info("open was successful\n");
    } else {
        pr_info("open was unsuccessful\n");
    }
    return ret;
}

int pcd_release (struct inode *inode, struct file *ptrFile){
    pr_info("close was successful\n");
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

static int __init pcd_driver_init(void){

    int ret;

    //Dynamically allocate a device number
    ret = alloc_chrdev_region(&pcdrvData.deviceNumber, 0, pcdrvData.totalDevices, "pcd_num");
    if (ret < 0){ 
        pr_err("chrdev failed\n");
        goto out;
    }

    //Create device file
    //Create device class in /sys/class/ for pcd device
    pcdrvData.classPCD = class_create("pcd_class_multiple");
    if(IS_ERR(pcdrvData.classPCD)){//When class create fails it return a pointer to error code which is type casted from a int which is assigned errno
        pr_err("Class creation failed\n");
        ret = PTR_ERR(pcdrvData.classPCD); //Converting pointer to error code
        goto unregCharDev;
    }

    for (int i =0; i<NO_OF_DEVICES; i++){
        
        pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(pcdrvData.deviceNumber+i), MINOR(pcdrvData.deviceNumber+i));
    
        //Initialiizing cdev structure
        cdev_init(&pcdrvData.pcdevData[i].myCdev, &pcd_fops); //THis will initialize the cdev variable's file operations field

        //Registering character device with virtual file system so that sys calls can be connected to driver
        pcdrvData.pcdevData[i].myCdev.owner = THIS_MODULE; //This is just a pointer to a strucure 
        ret = cdev_add(&pcdrvData.pcdevData[i].myCdev, pcdrvData.deviceNumber+i, 1);
        if (ret < 0){
            pr_err("cdev failed\n");
            goto cdevDel;
        }

        //Populate the sysfs with device information
        //Create sys/class/pcd_class/dev which can be read by udev system to crete device file in /dev/
        pcdrvData.pcdevData[i].devicePCD = device_create(pcdrvData.classPCD, NULL, pcdrvData.deviceNumber+i, NULL, "pcdev-%d", i);//"pcd" will appear in the dev directory
        if(IS_ERR(pcdrvData.pcdevData[i].devicePCD)){
            pr_err("Device creation failed\n");
            ret = PTR_ERR(pcdrvData.pcdevData[i].devicePCD);
            goto classDel;
        }
    }
    pr_info("PCD initialization done\n");
    return 0;

cdevDel:
classDel:
    for (int i = 0; i<NO_OF_DEVICES; i++){
        device_destroy(pcdrvData.classPCD, pcdrvData.deviceNumber+i);
        cdev_del(&pcdrvData.pcdevData[i].myCdev);
    }
    class_destroy(pcdrvData.classPCD);
unregCharDev:
    unregister_chrdev_region(pcdrvData.deviceNumber, NO_OF_DEVICES);
out:
    pr_err("Modle insertion failed\n");
    return ret;
}

static void __exit pcd_driver_cleanup(void){
    //The exit function will undo everything in reverse order that was initialized in init function
    //Destroy device in /dev/
    for (int i = 0; i<NO_OF_DEVICES; i++){
        device_destroy(pcdrvData.classPCD, pcdrvData.deviceNumber+i);
        //Delete the cdev struct
        cdev_del(&pcdrvData.pcdevData[i].myCdev);
    }
    //Destroy class in /sys/class/
    class_destroy(pcdrvData.classPCD);
    //Unregister character device
    unregister_chrdev_region(pcdrvData.deviceNumber,MAX_DEVICES);
    pr_info("PCD unloaded\n");
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