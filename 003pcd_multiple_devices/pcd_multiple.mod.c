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
	{ 0x122c3a7e, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x38e073, "class_create" },
	{ 0x5c658724, "cdev_init" },
	{ 0xa2dc7695, "cdev_add" },
	{ 0x23a131f9, "device_create" },
	{ 0x2f8479f2, "device_destroy" },
	{ 0xd58dd2b1, "cdev_del" },
	{ 0x9901e5e3, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x754d539c, "strlen" },
	{ 0x69acdf38, "memcpy" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xf079b8f9, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "A5541B7B370E05A2386E583");
