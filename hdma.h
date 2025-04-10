
#ifndef __HDMA_H_
#define __HDMA_H_

#include <stddef.h>
#include <asm/atomic.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include "eon_drv.h"
#include "test_dev.h"
#include "hdma.h"

#define IRAM_BASE_SOC_ADDR 0x48000000
#define HDMA_DESC_SOC_OFFSET_ADDR (0x10000) //64k desc start addr

#define HDMA_SIZE 0x800
#define HDMA_BASE_BAR_OFFSET 0x1000
//#define HDMA_SOC_OFFSET	0x80000
#define HDMA_SOC_OFFSET 0

#define HDMA_EN_OFF_WRCH(base, n) (base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x0)
#define HDMA_DOORBELL_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x4)
#define HDMA_ELEM_PF_OFF_WRCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x8)
#define HDMA_LLP_LOW_OFF_WRCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x10)
#define HDMA_LLP_HIGH_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x14)
#define HDMA_CYCLE_OFF_WRCH(base, n)                                           \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x18)
#define HDMA_XFERSIZE_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x1c)
#define HDMA_SAR_LOW_OFF_WRCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x20)
#define HDMA_SAR_HIGH_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x24)
#define HDMA_DAR_LOW_OFF_WRCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x28)
#define HDMA_DAR_HIGH_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x2c)
#define HDMA_WATERMARK_OFF_WRCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x30)
#define HDMA_CONTROL1_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x34)
#define HDMA_FUNC_NUM_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x38)
#define HDMA_QOS_OFF_WRCH(base, n)                                             \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x3c)
#define HDMA_STATUS_OFF_WRCH(base, n)                                          \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x80)
#define HDMA_INT_STATUS_OFF_WRCH(base, n)                                      \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x84)
#define HDMA_INT_SETUP_OFF_WRCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x88)
#define HDMA_INT_CLEAR_OFF_WRCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x8c)
#define HDMA_MSI_STOP_LOW_OFF_WRCH(base, n)                                    \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x90)
#define HDMA_MSI_STOP_HIGH_OFF_WRCH(base, n)                                   \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x94)
#define HDMA_MSI_WATERMARK_LOW_OFF_WRCH(base, n)                               \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x98)
#define HDMA_MSI_WATERMARK_HIGH_OFF_WRCH(base, n)                              \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0x9c)
#define HDMA_MSI_ABORT_LOW_OFF_WRCH(base, n)                                   \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0xa0)
#define HDMA_MSI_ABORT_HIGH_OFF_WRCH(base, n)                                  \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0xa4)
#define HDMA_MSI_MSGD_OFF_WRCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + n * HDMA_SIZE + 0xa8)

#define HDMA_EN_OFF_RDCH(base, n)                                              \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x0)
#define HDMA_DOORBELL_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x4)
#define HDMA_ELEM_PF_OFF_RDCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x8)
#define HDMA_LLP_LOW_OFF_RDCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x10)
#define HDMA_LLP_HIGH_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x14)
#define HDMA_CYCLE_OFF_RDCH(base, n)                                           \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x18)
#define HDMA_XFERSIZE_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x1c)
#define HDMA_SAR_LOW_OFF_RDCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x20)
#define HDMA_SAR_HIGH_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x24)
#define HDMA_DAR_LOW_OFF_RDCH(base, n)                                         \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x28)
#define HDMA_DAR_HIGH_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x2c)
#define HDMA_WATERMARK_OFF_RDCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x30)
#define HDMA_CONTROL1_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x34)
#define HDMA_FUNC_NUM_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x38)
#define HDMA_QOS_OFF_RDCH(base, n)                                             \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x3c)
#define HDMA_STATUS_OFF_RDCH(base, n)                                          \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x80)
#define HDMA_INT_STATUS_OFF_RDCH(base, n)                                      \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x84)
#define HDMA_INT_SETUP_OFF_RDCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x88)
#define HDMA_INT_CLEAR_OFF_RDCH(base, n)                                       \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x8c)
#define HDMA_MSI_STOP_LOW_OFF_RDCH(base, n)                                    \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x90)
#define HDMA_MSI_STOP_HIGH_OFF_RDCH(base, n)                                   \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x94)
#define HDMA_MSI_WATERMARK_LOW_OFF_RDCH(base, n)                               \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x98)
#define HDMA_MSI_WATERMARK_HIGH_OFF_RDCH(base, n)                              \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0x9c)
#define HDMA_MSI_ABORT_LOW_OFF_RDCH(base, n)                                   \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0xa0)
#define HDMA_MSI_ABORT_HIGH_OFF_RDCH(base, n)                                  \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0xa4)
#define HDMA_MSI_MSGD_OFF_RDCH(base, n)                                        \
	(base + HDMA_SOC_OFFSET + 0x400 + n * HDMA_SIZE + 0xa8)

#define BIT32_MASK GENMASK(31, 0)

struct dma_cmd_desc {
	uint8_t CB : 1;
	uint8_t reserved0 : 1;
	uint8_t LLP : 1;
	uint8_t LIE : 1;
	uint8_t RIE : 1;
	uint32_t reserved1 : 27;
	uint32_t transfer_size;
	uint32_t sar_low;
	uint32_t sar_high;
	uint32_t dar_low;
	uint32_t dar_high;
};

struct end_element {
	uint8_t CB : 1;
	uint8_t TCB : 1;
	uint8_t LLP : 1;
	uint32_t reserved0 : 29;
	uint32_t reserved1;
	uint32_t ll_pointer_low;
	uint32_t ll_pointer_high;
	uint32_t reserved[2];
};

//this is for desc
struct hdma_desc_phy_buf {
	dma_addr_t phy_addr;
	void *virt_addr;
	uint64_t start_pointer_addr;
	uint64_t size;
	atomic_t flag;
};

enum hdma_state {
	TRANSFER_STATE_IDLE = 0,
	TRANSFER_STATE_RUNNING,
	TRANSFER_STATE_COMPLETED,
	TRANSFER_STATE_FAILED,
};

struct hdma_regs {
	void __iomem *en;
	void __iomem *doorbell;
	void __iomem *elem_pf;
	void __iomem *llp_low;
	void __iomem *llp_high;
	void __iomem *cycle_off;
	void __iomem *xfer_size;
	void __iomem *sar_low;
	void __iomem *sar_high;
	void __iomem *dar_low;
	void __iomem *dar_high;
	void __iomem *watermark;
	void __iomem *control1;
	void __iomem *func_num; //pf vf number
	void __iomem *qos; //weight
	void __iomem *status;
	void __iomem *int_status;
	void __iomem *int_setup;
	void __iomem *int_clear;
	void __iomem *msi_stop_low;
	void __iomem *msi_stop_high;
	void __iomem *msi_watermark_low;
	void __iomem *msi_watermark_high;
	void __iomem *msi_abort_low;
	void __iomem *msi_abort_high;
	void __iomem *msi_msgd;
};


struct hdma_context {
	uint8_t id;
	struct hdma_regs ctx;
	struct eon_device *dpu_dev;
	spinlock_t lock;
	wait_queue_head_t wq;
	enum hdma_state state;
	uint32_t type;
	uint32_t flag;

	//for debug
	struct hdma_desc_phy_buf desc_buf;
	void *data;
};

void set_data_element(struct dma_cmd_desc *desc,
		      enum dma_transfer_direction dir, dma_addr_t src,
		      dma_addr_t dst, uint32_t len, uint8_t LIE, uint8_t RIE,
		      uint8_t CB);
void set_link_element(struct end_element *desc, dma_addr_t addr, uint8_t TCB,
		      uint8_t CB);

void set_cmd_desc1(struct dma_cmd_desc *desc, unsigned int len);
int hdma_lock(struct hdma_context *ctx);
int hdma_unlock(struct hdma_context *ctx);
int hdma_islocking(struct hdma_context *ctx);
void hdma_enable(struct hdma_context *ctx);
void hdma_disable(struct hdma_context *ctx);
int hdma_init(struct hdma_context **ctx, struct eon_device *pdev,
	      uint32_t channel, void __iomem *cfg_base, uint32_t rw_type);
int hdma_exit(struct hdma_context *ctx);
int hdma_start_transfer(struct hdma_context *ctx, uint32_t bound);
int hdma_terminal_transfer(struct hdma_context *ctx);
int hdma_set_desc_pointer(struct hdma_context *ctx, uint64_t addr);
void hdma_msi_interrupt(struct hdma_context *ctx, uint32_t msi_addr_h,
			uint32_t msi_addr_l, uint16_t msg_data);
int hdma_set_attr(struct hdma_context *ctx, uint32_t control1, uint32_t pf,
		  uint32_t cycle_off);
void set_xfer_data(struct hdma_context *ctx, uint32_t xfer_size, uint64_t src,
		   uint64_t des);
uint32_t hdma_get_intr_status(struct hdma_context *ctx);
void hdma_clear_intr_status(struct hdma_context *ctx, uint32_t val);
int hdma_enable_intr(struct hdma_context *ctx, uint32_t mask);
int hdma_disable_intr(struct hdma_context *ctx);

void dump_desc(struct dma_cmd_desc desc);

#endif
