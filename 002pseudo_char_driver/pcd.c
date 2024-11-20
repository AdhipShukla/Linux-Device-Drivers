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

#define DEV_MEM_SIZE 512
#undef  pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name

//Pseudo Device Memory
char device_buffer[DEV_MEM_SIZE];

//Variable to hold device number
dev_t device_number;

//Cdev Variable one for each device
struct cdev pcd_cdev;

//Function decalration
loff_t pcd_llseek (struct file *ptrFile, loff_t off, int whence);
ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos);
ssize_t pcd_write (struct file *ptrFile, const char __user *buff, size_t count, loff_t *fPos);
int pcd_open (struct inode *inode, struct file *ptrFile);
int pcd_release (struct inode *inode, struct file *ptrFile);

/**
 * File Operations
 */

loff_t pcd_llseek (struct file *ptrFile, loff_t offset, int whence){
    
    pr_info("llseek requested\n");
    loff_t tempOffset = 0;
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    if(whence == SEEK_SET){
        tempOffset = offset;
        if(tempOffset > DEV_MEM_SIZE || tempOffset<0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else if (whence == SEEK_CUR) {
        tempOffset = ptrFile->f_pos + offset;
        if(tempOffset > DEV_MEM_SIZE || tempOffset<0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else if (whence == SEEK_END) {
        tempOffset = DEV_MEM_SIZE + offset;
        if(tempOffset > DEV_MEM_SIZE || tempOffset<0){
            return -EINVAL;
        }
        ptrFile->f_pos = tempOffset;
    } else {
        return -EINVAL; //Invalid value passed for whence
    }
    return ptrFile->f_pos;
}

ssize_t pcd_read (struct file *ptrFile, char __user *buff, size_t count, loff_t *fPos){
    
    pr_info("read requested for %zu bytes\n", count);
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    /*Adjusting the number of bytes to be read*/
    if(*fPos + count > DEV_MEM_SIZE){
        count = DEV_MEM_SIZE - *fPos;
    }
    /*Copying the byte to user address from the pseudo device*/
    long bytesNotCopied = copy_to_user(buff, &device_buffer[*fPos], count);
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
    pr_info("write requested for %zu bytes\n", count);
    pr_info("Current value of the file pointer position is %lld", ptrFile->f_pos);
    /*Adjusting the number of bytes to be written*/
    if(*fPos + count > DEV_MEM_SIZE){
        count = DEV_MEM_SIZE - *fPos;
    }
    /*If Count is zero*/
    if(!count){
        pr_err("No space left on pcd device\n");
        return -ENOMEM;
    }
    /*Copying the byte from user address to pseudo device*/
    long bytesNotCopied = copy_from_user(&device_buffer[*fPos], buff, count);
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

int pcd_open (struct inode *inode, struct file *ptrFile){
    pr_info("open was successful\n");
    return 0;
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

struct class *class_pcd;

struct device *device_pcd;

static int __init pcd_driver_init(void){

    int ret;

    //Dynamically allocate a device number
    ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_num");
    if (ret < 0){ 
        pr_err("chrdev failed\n");
        goto out;
    }
    pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));

    //Initialiizing cdev structure
    cdev_init(&pcd_cdev, &pcd_fops); //THis will initialize the cdev variable's file operations field

    //Registering character device with virtual file system so that sys calls can be connected to driver
    pcd_cdev.owner = THIS_MODULE; //This is just a pointer to a strucure 
    ret = cdev_add(&pcd_cdev, device_number, 1);
    if (ret < 0){
        pr_err("cdev failed\n");
        goto unregCharDev;
    }

    //Create device file
    //Create device class in /sys/class/ for pcd device
    class_pcd = class_create("pcd_class");
    if(IS_ERR(class_pcd)){//When class create fails it return a pointer to error code which is type casted from a int which is assigned errno
        pr_err("Class creation failed\n");
        ret = PTR_ERR(class_pcd); //Converting pointer to error code
        goto cdevDelete;
    }

    //Populate the sysfs with device information
    //Create sys/class/pcd_class/dev which can be read by udev system to crete device file in /dev/
    device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");//"pcd" wil appear in the dev directory
    if(IS_ERR(device_pcd)){
        pr_err("Device creation failed\n");
        ret = PTR_ERR(device_pcd);
        goto classDel;
    }

    pr_info("PCD initialization done\n");
    return 0;

classDel:
    class_destroy(class_pcd);
cdevDelete:
    cdev_del(&pcd_cdev);
unregCharDev:
    unregister_chrdev_region(device_number, 1);
out:
    pr_err("Modle insertion failed\n");
    return ret;
}

static void __exit pcd_driver_cleanup(void){
    //The exit function will undo everything in reverse order that was initialized in init function
    //Destroy device in /dev/
    device_destroy(class_pcd, device_number);
    //Destroy class in /sys/class/
    class_destroy(class_pcd);
    //Delete the cdev struct
    cdev_del(&pcd_cdev);
    //Unregister character device
    unregister_chrdev_region(device_number,1);
    
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