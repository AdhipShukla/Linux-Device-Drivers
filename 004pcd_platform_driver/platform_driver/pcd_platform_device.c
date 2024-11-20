/**********************************************************************************************
* Header Section
***********************************************************************************************/

#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"
/**********************************************************************************************
* Code Section
***********************************************************************************************/
void pcdev_release(struct device *dev);
void pcdev_release(struct device *dev){
    pr_info("Nothing to do, device released\n");
}
struct pcdev_platform_data pcdevPlatformData[2] = {
    [0] = {
        .size = 512,
        .permissions =RDWR,
        .serialNumber = "PCDDEV1"
    },
    [1] = {
        .size = 1024,
        .permissions =RDWR,
        .serialNumber = "PCDDEV2"
    }
};

struct platform_device platformPcdev1 = {
    .name = "pseudo-char-device",
    .id = 0,
    .dev = {
        .platform_data = &pcdevPlatformData[0],
        .release =pcdev_release
    }
};

struct platform_device platformPcdev2 = {
    .name = "pseudo-char-device",
    .id = 1,
    .dev = {
        .platform_data = &pcdevPlatformData[1],
        .release =pcdev_release
    }
};

static int __init pcdev_platform_init(void){
    platform_device_register(&platformPcdev1);
    platform_device_register(&platformPcdev2);
    pr_info("Device setup module inserted\n");
    return 0;
}

static void __exit pcdev_platform_exit(void){
    platform_device_unregister(&platformPcdev1);
    platform_device_unregister(&platformPcdev2);
    pr_info("Device setup module removed\n");
}

/**********************************************************************************************
* Resgistration Section
***********************************************************************************************/

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

/**********************************************************************************************
* Description Section
***********************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adhip");
MODULE_DESCRIPTION("Pseudo Character Platform Device Setup");
MODULE_INFO(board, "BBB");