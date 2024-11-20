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
#include <linux/gpio/consumer.h>

/**********************************************************************************************
* Private Functions
***********************************************************************************************/

int gpio_sysfs_probe(struct platform_device *pdev);
int gpio_sysfs_remove(struct platform_device *pdev);
int __init gpio_sysfs_init(void);
void __exit gpio_sysfs_cleanup(void);
ssize_t direction_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf);

/**********************************************************************************************
* Typedefs
***********************************************************************************************/

//Macros
#undef  pr_fmt
#define pr_fmt(fmt)         "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name

//Device private data structure
struct gpiodev_private_data {
    char label[20];
    struct gpio_desc *desc;
};

//Driver private data structure
struct gpiodrv_private_data {
    int totalDevices;
    struct class *classGPIO;
};

/**********************************************************************************************
* Main Body
***********************************************************************************************/

//Struct Variables
struct  gpiodrv_private_data gpioDrvData;
struct device *deviceSysfs;

//Attributes Definition
static DEVICE_ATTR_RW(direction); //This marco is a cleaner way of defining an attribute which has both show and store methods 
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute *gpioAttrs[] = { //Defining a list of attributes
    &dev_attr_direction.attr,
    &dev_attr_value.attr,
    &dev_attr_label.attr,
    NULL
};

static struct attribute_group gpioAttrGroup = { //Creating groups of attributes, structure defined in sysfs.h
    .attrs = gpioAttrs,
};

static const struct attribute_group *gpioAttrGroups[] = { // Defining a list of attribute groups
    &gpioAttrGroup,
    NULL
};

ssize_t direction_show(struct device *dev, struct device_attribute *attr, char *buf){
    return 0;
}

ssize_t direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    return 0;
}

ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf){
    return 0;
}

ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    return 0;
}

ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf){
    return 0;
}

//Probe Function
int gpio_sysfs_probe(struct platform_device *pdev){
    
    struct device_node *parent = pdev->dev.of_node;//Parent Device Node
    struct device_node *child = NULL;
    struct device *device= &pdev->dev;
    struct gpiodev_private_data *deviceData;
    const char *name;
    int i = 0;
    int ret;
    for_each_available_child_of_node(parent,child){//This is a macro in of.h which iterates over all the child nodes of a parent node and stops at NULL node. In this case the gpio childs in dtsi
        deviceData = devm_kzalloc(device,sizeof(*deviceData),GFP_KERNEL);
        if(!deviceData){
            dev_err(device, "Cannot allocate memory\n");
            return -ENOMEM;
        }

        if(of_property_read_string(child,"label",&name)){ //This API fetches the dtsi property named as 2nd argument paased, for the device node and returns 0 on success
            dev_warn(device, "Missing label information\n");
            snprintf(deviceData->label,sizeof(deviceData->label),"unknowngpio%d",i);
        } else {
            strcpy(deviceData->label,name);
            dev_info(device,"GPIO label = %s\n",deviceData->label);
        }

        //This API aquires the GPIO descriptor associated with a specific child node. The argumrnts include parent device, connection id mentioned in dtb, also called the gpio function, child node, flag showing how to access, and label for debugging.
        //devm_fwnode_get_gpiod_from_child(device, "bone", &child->fwnode, GPIOD_ASIS, deviceData->label);
        deviceData->desc = devm_fwnode_gpiod_get_index(device, &child->fwnode, "bone", 0, GPIOD_ASIS, deviceData->label);
        if(IS_ERR(deviceData->desc)){
            ret = PTR_ERR(deviceData->desc);
            if(ret == -ENOENT){
                dev_err(device,"No GPIO has been assigned to the requested function and/or index\n");
            }
            return ret;
        }
        //Set the GPIO direction to the output providing the current state of 0(meaning low in non-active low state)
        ret = gpiod_direction_output(deviceData->desc,0);
        if(ret){
            dev_err(device,"gpio direction set failed\n");
            return ret;
        }

        //Creating devices for all the child nodes under sys/class/bone_gpio this API eliminates the need to create separate devices and attributes
        deviceSysfs = device_create_with_groups(gpioDrvData.classGPIO, device, 0, deviceData, gpioAttrGroups, deviceData->label); //Passing device number results in files not created in /dev/ as we just want sysfs files
        if(IS_ERR(deviceSysfs)){
            dev_err(device, "Error in device_create\n");
            return PTR_ERR(deviceSysfs);
        }

        i++;
    }
    return 0;
}
//Remove Function
int gpio_sysfs_remove(struct platform_device *pdev){
    return 0;
}

//Init Fucntions
struct of_device_id gpioDeviceMatch[] = {
    {.compatible = "org,bone-gpio-sysfs"}, //This compatible property value is coming from the device tree binding informatoin
    {}
};
struct platform_driver gpioSysFs_PlatformDriver = {
    .probe = gpio_sysfs_probe,
    .remove = gpio_sysfs_remove,
    .driver = {
        .name = "bone-gpio-sysfs",
        .of_match_table = of_match_ptr(gpioDeviceMatch),
    }
};

int __init gpio_sysfs_init(void){
    gpioDrvData.classGPIO = class_create("bone_gpios");
    if(IS_ERR(gpioDrvData.classGPIO)){
        pr_err("Error in creating class \n");
        return PTR_ERR(gpioDrvData.classGPIO);
    }
    platform_driver_register(&gpioSysFs_PlatformDriver);
    pr_info("Module loaded successfully");
    return 0;
}

void __exit gpio_sysfs_cleanup(void){

    platform_driver_unregister(&gpioSysFs_PlatformDriver);
    class_destroy(gpioDrvData.classGPIO);
}



/**********************************************************************************************
* Resgistration Section
***********************************************************************************************/

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_cleanup);

/**********************************************************************************************
* Description Section
***********************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adhip");
MODULE_DESCRIPTION("GPIO Sysfs Driver");
MODULE_INFO(board, "BBB");