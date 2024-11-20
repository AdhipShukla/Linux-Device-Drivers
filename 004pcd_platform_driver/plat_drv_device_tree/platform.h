
#undef  pr_fmt
#define pr_fmt(fmt)         "%s : " fmt,__func__ //Updating the input for printk called from pr_infor to include fucntion name
#define RDONLY  0X01
#define WRONLY  0X10
#define RDWR    0X11

struct pcdev_platform_data{
    unsigned size;
    const char *serialNumber;
    int permissions;
};