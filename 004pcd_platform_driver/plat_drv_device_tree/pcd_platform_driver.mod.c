#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x1899caca, "__current" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0xba92102f, "device_destroy" },
	{ 0x5f5f7222, "cdev_del" },
	{ 0x37a0cba, "kfree" },
	{ 0x11194372, "_dev_info" },
	{ 0xad942f78, "platform_driver_unregister" },
	{ 0xa586ca7b, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xdcc712ea, "class_create" },
	{ 0xacd273b3, "__platform_driver_register" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x5f754e5a, "memset" },
	{ 0xd2ed9054, "devm_kmalloc" },
	{ 0x11f23c76, "of_property_read_string" },
	{ 0xec7e83c9, "of_property_read_variable_u32_array" },
	{ 0x41587c28, "of_match_device" },
	{ 0xc3d6c279, "kmalloc_trace" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0x70e7658a, "cdev_init" },
	{ 0x6940f29a, "cdev_add" },
	{ 0x4b119ba5, "device_create" },
	{ 0xe0fca2b0, "_dev_err" },
	{ 0x1584fb2f, "kmalloc_caches" },
	{ 0xa10d3b15, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "24A7B45DD6AF9F6E737E740");
