/**********************************************************************************************
* Header Section
***********************************************************************************************/

#include <linux/kernel.h>
#include<linux/module.h>

/**********************************************************************************************
* Code Section
***********************************************************************************************/

//Module initialization entry point. Called furing boot time in case of static modules and during insertion in case of dynamic module
// #define __init __section(.init.text) [this makes sure init function is placed in init section of elf, wchih can be removed after first loading]
static int __init helloworld_init(void){
    pr_info("Hello World\n"); //Macro wrapper for printk
    return 0;
}

//Module entry point when module is removed this function is only used in dynamic modules, not required for static modules which can't be removed
// #define __exit __section(.exit.text)
static void __exit helloworld_cleanup(void){
    pr_info("Good Bye World\n");
}

/**********************************************************************************************
* Resgistration Section
***********************************************************************************************/

module_init(helloworld_init);
module_exit(helloworld_cleanup);

/**********************************************************************************************
* Description Section
***********************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adhip");
MODULE_DESCRIPTION("Hello World from Kernel");
MODULE_INFO(board, "Beaglebone black");