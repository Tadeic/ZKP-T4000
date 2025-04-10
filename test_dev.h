#ifndef _TEST_DEV_H_
#define _TEST_DEV_H_

#include "eon_drv.h"
#include <linux/cdev.h>
#include <linux/compiler.h>
#include <linux/ioctl.h>

#define ENABLE_HDMA_TIMEOUT
#define HDMA_TIMEOUT_LENGTH		5000 //ms

#define REGS_ADDR_MIN (0x40000000ul)
#define REGS_ADDR_MAX (0xf0000000ul)

#define CONFIG_BAR_SIZE (0x1000000)

#define COMMAND_START 0x1
#define COMMAND_END 0x1

#define COMMAND_HDMA_INBOUND 1
#define COMMAND_HDMA_OUTBOUND 2

#define REG_OPERATION_MAX_NUM 0x10

struct dpu_page_blocks {
	struct page *pages;
	uint32_t pages_nr;
};

struct dpu_host_mem_info {
	uint64_t addr;
	uint64_t len;
	struct dpu_page_blocks *blocks; //point to a array
	uint32_t blocks_nr;
	//int block_type;
};

struct xfer_job {
	uint64_t src;
	uint64_t dst;
	uint64_t size;
};

struct dpu_xfer_info {
	struct sg_table *sgt;
	unsigned int sglen;
	struct xfer_job align;
	struct dpu_host_mem_info hmem_info;

	uint64_t desc_addr; //the req desc_addr
	uint8_t xfer_opt;
	bool done;
	uint64_t size;
	uint64_t dir;
};

enum dma_operation {
	dma_inbound = 1,
	dma_outbound,
};

enum dma_ctrl {
	dma_disable = 0,
	dma_enable,
};

enum reg_ctrl {
	reg_disable = 0,
	reg_enable,
};

struct reg_addr_data{
	uint64_t reg_addr;
	uint32_t reg_data;
};

struct reg_infos{
	uint8_t reg_operation;
	uint8_t num;

	struct reg_addr_data write_data[REG_OPERATION_MAX_NUM];
};

struct hdma_mem_args {
	uint64_t host_addr;
	uint64_t device_addr;
	uint64_t mem_size;
	uint64_t desc_addr;
	uint32_t hdma_id;
	uint32_t op;
	uint32_t enable;

	struct reg_infos regs;
	uint32_t reserved[4];
};

struct dpu_desc_info {
	uint64_t desc_num;
	uint64_t desc_size;
	uint64_t desc_host_addr;
	uint64_t desc_addr;
	uint64_t desc_device_addr;
	int dir;
};

#define EON_IOCTL_TEST _IO('S', 0x01)
#define EON_IOCTL_TEST_ERR _IO('S', 0x05)
#define EON_IOCTL_IRAM_CPYTEST _IO('S', 0x02)
#define EON_IOCTL_HDMA _IOWR('S', 0x03, struct hdma_mem_args)
#define EON_IOCTL_IATU _IOWR('S', 0x04, struct dpu_reg_args)

#define REG_WRITE 1
#define REG_READ 2
#define REG_WRITE16 3
#define REG_READ16 4
struct dpu_reg_args {
	uint64_t reg_addr;
	uint32_t reg_data;
	uint8_t rw_flag;
	uint32_t reserved[4];
};

struct char_dev_ctx {
	struct cdev dev;
	struct device *char_dev;
	struct eon_device *dpu_dev;
};

int char_dev_init(struct eon_device *eon);
int char_dev_fini(struct eon_device *eon);
#endif
