#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

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
	{ 0x6ad771c3, "module_layout" },
	{ 0x764cadad, "dma_map_sg_attrs" },
	{ 0x11eb121f, "cdev_del" },
	{ 0xd69e5a4f, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xd90cd7e6, "cdev_init" },
	{ 0x7a8b29c7, "put_devmap_managed_page" },
	{ 0x13d0adf7, "__kfifo_out" },
	{ 0x8b5bc5b3, "pci_free_irq_vectors" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x5c8781db, "pcim_enable_device" },
	{ 0x139f2189, "__kfifo_alloc" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0x80c98300, "pcim_iomap_table" },
	{ 0x1e6f1b87, "dma_set_mask" },
	{ 0x7f5e2cc, "pcie_set_readrq" },
	{ 0x56470118, "__warn_printk" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x15659e9d, "device_destroy" },
	{ 0x87b8798d, "sg_next" },
	{ 0x3ef73b4d, "pcie_capability_clear_and_set_word" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x999e8297, "vfree" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x33a21a09, "pv_ops" },
	{ 0x6eec4dac, "dma_set_coherent_mask" },
	{ 0xd10a0a5f, "kthread_create_on_node" },
	{ 0xfb384d37, "kasprintf" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb24af53b, "pci_set_master" },
	{ 0x5be158a5, "pci_alloc_irq_vectors_affinity" },
	{ 0xfb578fc5, "memset" },
	{ 0xfbb71e91, "default_llseek" },
	{ 0xa22a96f7, "current_task" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x8a23e3a5, "kthread_stop" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0x648d9e84, "pci_read_config_word" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x70eb7a97, "device_create" },
	{ 0xac7ab837, "_dev_err" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xdd9eb7b7, "pcim_iomap_regions" },
	{ 0x646eac6, "cdev_add" },
	{ 0x800473f, "__cond_resched" },
	{ 0x3a2f6702, "sg_alloc_table" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x8c2b27fe, "devm_free_irq" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x92997ed8, "_printk" },
	{ 0x461f3cc3, "dma_map_page_attrs" },
	{ 0x559bf800, "pci_read_config_dword" },
	{ 0xb06a469c, "dev_driver_string" },
	{ 0x3d37116f, "wake_up_process" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xc0b2bc48, "pci_unregister_driver" },
	{ 0x4f00afd3, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xdb760f52, "__kfifo_free" },
	{ 0x45a2728c, "pci_irq_vector" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x6a86bc1, "iowrite16" },
	{ 0x37a0cba, "kfree" },
	{ 0x433f0b06, "__pci_register_driver" },
	{ 0xb3f0559, "class_destroy" },
	{ 0x4b4ce6ce, "dma_unmap_page_attrs" },
	{ 0x842c8e9d, "ioread16" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x7f5b4fe4, "sg_free_table" },
	{ 0xf23fcb99, "__kfifo_in" },
	{ 0x4a453f53, "iowrite32" },
	{ 0x970f882b, "devm_kmalloc" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xa65a29a4, "devm_request_threaded_irq" },
	{ 0x52ea150d, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xf9818c63, "__put_page" },
	{ 0xa78af5f3, "ioread32" },
	{ 0x71ac56b2, "get_user_pages_fast" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
	{ 0x587f22d7, "devmap_managed_key" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v000016C3d0000ABCDsv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "91FE7A5218028A561AB7C52");
