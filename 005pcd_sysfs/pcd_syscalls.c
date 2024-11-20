#include"pcd_platform_driver_sysfs.h"

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