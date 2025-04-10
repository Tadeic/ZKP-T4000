#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0xc117a4e9, "module_layout" },
	{ 0x6b68aa8e, "cdev_del" },
	{ 0x4320c0cf, "kmalloc_caches" },
	{ 0xd2b09ce5, "__kmalloc" },
	{ 0x1ace07ff, "cdev_init" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0x13d0adf7, "__kfifo_out" },
	{ 0xc8353969, "pci_free_irq_vectors" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x320092be, "pcim_enable_device" },
	{ 0x97868aef, "__kfifo_alloc" },
	{ 0xb5f02981, "pcim_iomap_table" },
	{ 0x9b470e2d, "pcie_set_readrq" },
	{ 0x84bc974b, "__arch_copy_from_user" },
	{ 0x9b7fe4d4, "__dynamic_pr_debug" },
	{ 0xc969af56, "device_destroy" },
	{ 0x87b8798d, "sg_next" },
	{ 0x751886ca, "pcie_capability_clear_and_set_word" },
	{ 0xa6093a32, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x999e8297, "vfree" },
	{ 0xaa494c71, "kthread_create_on_node" },
	{ 0x44b5ee9a, "kasprintf" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x9cc0c0a9, "pci_set_master" },
	{ 0xe154a2da, "pci_alloc_irq_vectors_affinity" },
	{ 0xdcb764ad, "memset" },
	{ 0xc3f0f1b1, "default_llseek" },
	{ 0x9a76f11f, "__mutex_init" },
	{ 0x7c32d0f0, "printk" },
	{ 0x10e53428, "kthread_stop" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xc57ac5e1, "pci_read_config_word" },
	{ 0x41aed6e7, "mutex_lock" },
	{ 0xd7f05c49, "device_create" },
	{ 0xbd48a0c5, "_dev_err" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xfc52334, "pcim_iomap_regions" },
	{ 0x62c9ae84, "cdev_add" },
	{ 0x5666192c, "sg_alloc_table" },
	{ 0x8142d5f, "devm_free_irq" },
	{ 0xb35dea8f, "__arch_copy_to_user" },
	{ 0x9c1e5bf5, "queued_spin_lock_slowpath" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xddf19777, "pci_read_config_dword" },
	{ 0x3eef279, "cpu_hwcap_keys" },
	{ 0xaafb1e9, "wake_up_process" },
	{ 0x6a80bb3c, "pci_unregister_driver" },
	{ 0xe4c41e88, "kmem_cache_alloc_trace" },
	{ 0x4e3dee1f, "dummy_dma_ops" },
	{ 0xdb760f52, "__kfifo_free" },
	{ 0xac72d73f, "pci_irq_vector" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x37a0cba, "kfree" },
	{ 0x3321e9ba, "__pci_register_driver" },
	{ 0x9c3c5d64, "class_destroy" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x7f5b4fe4, "sg_free_table" },
	{ 0xf23fcb99, "__kfifo_in" },
	{ 0xbb3090c7, "devm_kmalloc" },
	{ 0x6261a246, "devm_request_threaded_irq" },
	{ 0x8fc43912, "__class_create" },
	{ 0x6dfb912f, "arm64_const_caps_ready" },
	{ 0xc713890e, "flush_dcache_page" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x45f4e3c9, "__put_page" },
	{ 0x769a7693, "get_user_pages_fast" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v000016C3d0000ABCDsv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "CB26C02552BF04BC3780DA7");
