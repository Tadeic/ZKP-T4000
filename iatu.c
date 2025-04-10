#include "iatu.h"
#include "hdma.h"

#define BAR_SIZE_MASK 0x1ffffff

//max size 16M
int iatu_map_outbound_bar(struct eon_device *eon_device, uint8_t bar_index,
			  uint8_t atu_index, uint64_t addr, uint64_t size)
{
	uint64_t base = (uint64_t)eon_device->bar[ATU_HDMA_BAR].vaddr +
			IATU_BASE_OFFSET;
	int ret = 0;

	if (size > 0x1000000) {
		return -1;
	}

	eon_writel(addr & BIT32_MASK,
		   (void *)IATU_LWR_TARGET_ADDR_OUTBOUND(base, atu_index));
	eon_writel((addr >> 32) & BIT32_MASK,
		   (void *)IATU_UPPER_TARGET_ADDR_OUTBOUND(base, atu_index));
	eon_writel(size & BAR_SIZE_MASK,
		   (void *)IATU_REGION_CTRL_1_OUTBOUND(base, atu_index));
	eon_writel(0xc0000000 | (bar_index << 8),
		   (void *)IATU_REGION_CTRL_2_OUTBOUND(base, atu_index));
	eon_readl((void *)IATU_REGION_CTRL_2_OUTBOUND(base, atu_index));

	dpu_dbg("%s %d++\n", __func__, __LINE__);

	return ret;
}

int iatu_map_inbound_bar(struct eon_device *eon_device, uint8_t bar_index,
			 uint8_t atu_index, uint64_t addr, uint64_t size)
{
	uint64_t base = (uint64_t)eon_device->bar[ATU_HDMA_BAR].vaddr +
			IATU_BASE_OFFSET;
	int ret = 0;

	if (size > 0x1000000) {
		return -1;
	}

	eon_writel(addr & BIT32_MASK,
		   (void *)IATU_LWR_TARGET_ADDR_INBOUND(base, atu_index));
	eon_writel((addr >> 32) & BIT32_MASK,
		   (void *)IATU_UPPER_TARGET_ADDR_INBOUND(base, atu_index));
	eon_writel(size & BAR_SIZE_MASK,
		   (void *)IATU_REGION_CTRL_1_INBOUND(base, atu_index));
	eon_writel(0xc0000000 | (bar_index << 8),
		   (void *)IATU_REGION_CTRL_2_INBOUND(base, atu_index));
	eon_readl((void *)IATU_REGION_CTRL_2_INBOUND(base, atu_index));

	dpu_dbg("%s %d++\n", __func__, __LINE__);

	return ret;
}
