#ifndef __EON_DRV_H
#define __EON_DRV_H

#include <linux/pci.h>
#include <linux/rwlock.h>
#include <linux/printk.h>
#include <linux/kfifo.h>

#define DEBUG

#define dpu_err(format, ...)                                                   \
	({                                                                     \
		pr_err("%s:%d:(%s tgid %d pid %d): ERROR " format, __func__,   \
		       __LINE__, current->comm, current->tgid, current->pid,   \
		       ##__VA_ARGS__);                                         \
	})

/*******
#define dpu_dbg(format, ...)                                                   \
	pr_debug("%s:%d:(%s %d %d %d): " format, __func__, __LINE__,              \
		 current->comm, current->pid, current->tgid, smp_processor_id(), ##__VA_ARGS__)
***/

#define dpu_dbg(format, ...)                                                   \
	pr_debug("%s:%d:(%s %d): " format, __func__, __LINE__, current->comm,   \
		current->pid, ##__VA_ARGS__)
#define dpu_info(format, ...)                                                  \
	pr_info("%s:%d:(%s %d): " format, __func__, __LINE__, current->comm,   \
		current->pid, ##__VA_ARGS__)
#define dpu_warn(format, ...)                                                  \
	pr_warn("%s:%d:(%s %d): " format, __func__, __LINE__, current->comm,   \
		current->pid, ##__VA_ARGS__)

#define EON_TEST_MINORS 8
#define EON_TEST "eon_test"

#define EON_NAME "eon-dpu"
#define EON_VENDOR_ID 0x16c3
#define EON_DEVICE_ID 0xabcd

#define EON_MSI_IRQ_NUM 1
#define EON_BAR_NUM 6
#define ATU_HDMA_BAR 0
#define CONFIG_BAR_INDEX 2
#define DESC_BAR 4

#define MAX_HDMA_NUMBER 16

struct bar_info {
	phys_addr_t paddr;
	void __iomem *vaddr;
	size_t sz;
};

struct addr_info {
	void __iomem *iatu_base;
	void __iomem *hdma_base;

	uint32_t msi_addr_l;
	uint32_t msi_addr_h;
	uint16_t msg_data;
};

struct eon_device {
	struct pci_dev *pdev;
	struct bar_info bar[6];
	uint8_t irqs;
	struct addr_info base_addr;

	uint32_t dpu_id;
	struct char_dev_ctx *char_ctx;
	struct hdma_context *hdmar[MAX_HDMA_NUMBER];
	struct hdma_context *hdmaw[MAX_HDMA_NUMBER];

	uint64_t config_bar_base;
	struct mutex conbar_lock;

	struct task_struct *hdma_thread;
	struct kfifo hdma_kfifo;
	wait_queue_head_t thread_wq;
	uint32_t should_stop_flag;
};

uint32_t eon_readl(void *addr);
void eon_writel(unsigned int data, void *addr);
uint16_t eon_read16(void *addr);
void eon_write16(uint16_t data, void *addr);
void hdma_writel(void* hdma_base, unsigned int data, void *addr);

#endif
