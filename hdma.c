#include <asm/atomic.h>
#include "hdma.h"
#include "eon_drv.h"

void set_data_element(struct dma_cmd_desc *desc,
		      enum dma_transfer_direction dir, dma_addr_t src,
		      dma_addr_t dst, uint32_t len, uint8_t LIE, uint8_t RIE,
		      uint8_t CB)
{
	desc->CB = CB;
	desc->LLP = 0;
	desc->LIE = LIE;
	desc->RIE = RIE;

	// dpu_dbg("desc addr %llx src %llx des %llx len %x\n", (uint64_t)desc,
	// 	src, dst, len);

	desc->dar_low = dst & BIT32_MASK;
	desc->dar_high = (dst >> 32) & BIT32_MASK;
	desc->sar_low = src & BIT32_MASK;
	desc->sar_high = (src >> 32) & BIT32_MASK;
	desc->transfer_size = len;
}

void set_link_element(struct end_element *desc, dma_addr_t addr, uint8_t TCB,
		      uint8_t CB)
{
	dpu_dbg("desc addr %llx addr %llx \n", (uint64_t)desc, addr);
	desc->CB = CB;
	desc->TCB = TCB;
	desc->LLP = 1;
	desc->ll_pointer_low = addr & BIT32_MASK;
	desc->ll_pointer_high = (addr >> 32) & BIT32_MASK;
}

int hdma_lock(struct hdma_context *ctx)
{
	spin_lock(&ctx->lock);
	if (ctx->flag == 1) {
		spin_unlock(&ctx->lock);
		return -1;
	} else {
		ctx->flag = 1;
	}
	spin_unlock(&ctx->lock);

	return 0;
}

int hdma_unlock(struct hdma_context *ctx)
{
	spin_lock(&ctx->lock);
	ctx->flag = 0;
	spin_unlock(&ctx->lock);
	return 0;
}

int hdma_islocking(struct hdma_context *ctx)
{
	return ctx->flag;
}

int hdma_init(struct hdma_context **ctx, struct eon_device *dpu_dev,
	      uint32_t channel, void __iomem *cfg_base, uint32_t rw_type)
{
	struct hdma_context *tmp_ctx;

	if (!ctx) {
		pci_err(dpu_dev->pdev, "hdma_init error\n");
		return -EPERM;
	}

	tmp_ctx = kzalloc(sizeof(struct hdma_context), GFP_KERNEL);
	if (unlikely(!tmp_ctx))
		return -EPERM;
	
	tmp_ctx->id = channel;

	if (rw_type == 1) {
		tmp_ctx->ctx.en = (void *)HDMA_EN_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.doorbell =
			(void *)HDMA_DOORBELL_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.elem_pf =
			(void *)HDMA_ELEM_PF_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.llp_low =
			(void *)HDMA_LLP_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.llp_high =
			(void *)HDMA_LLP_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.cycle_off =
			(void *)HDMA_CYCLE_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.xfer_size =
			(void *)HDMA_XFERSIZE_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.sar_low =
			(void *)HDMA_SAR_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.sar_high =
			(void *)HDMA_SAR_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.dar_low =
			(void *)HDMA_DAR_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.dar_high =
			(void *)HDMA_DAR_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.watermark =
			(void *)HDMA_WATERMARK_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.control1 =
			(void *)HDMA_CONTROL1_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.func_num =
			(void *)HDMA_FUNC_NUM_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.qos = (void *)HDMA_QOS_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.status =
			(void *)HDMA_STATUS_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.int_status =
			(void *)HDMA_INT_STATUS_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.int_setup =
			(void *)HDMA_INT_SETUP_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.int_clear =
			(void *)HDMA_INT_CLEAR_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_stop_low =
			(void *)HDMA_MSI_STOP_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_stop_high =
			(void *)HDMA_MSI_STOP_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_watermark_low =
			(void *)HDMA_MSI_WATERMARK_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_watermark_high =
			(void *)HDMA_MSI_WATERMARK_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_abort_low =
			(void *)HDMA_MSI_ABORT_LOW_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_abort_high =
			(void *)HDMA_MSI_ABORT_HIGH_OFF_WRCH(cfg_base, channel);
		tmp_ctx->ctx.msi_msgd =
			(void *)HDMA_MSI_MSGD_OFF_WRCH(cfg_base, channel);
	} else {
		tmp_ctx->ctx.en = (void *)HDMA_EN_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.doorbell =
			(void *)HDMA_DOORBELL_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.elem_pf =
			(void *)HDMA_ELEM_PF_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.llp_low =
			(void *)HDMA_LLP_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.llp_high =
			(void *)HDMA_LLP_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.cycle_off =
			(void *)HDMA_CYCLE_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.xfer_size =
			(void *)HDMA_XFERSIZE_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.sar_low =
			(void *)HDMA_SAR_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.sar_high =
			(void *)HDMA_SAR_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.dar_low =
			(void *)HDMA_DAR_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.dar_high =
			(void *)HDMA_DAR_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.watermark =
			(void *)HDMA_WATERMARK_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.control1 =
			(void *)HDMA_CONTROL1_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.func_num =
			(void *)HDMA_FUNC_NUM_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.qos = (void *)HDMA_QOS_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.status =
			(void *)HDMA_STATUS_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.int_status =
			(void *)HDMA_INT_STATUS_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.int_setup =
			(void *)HDMA_INT_SETUP_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.int_clear =
			(void *)HDMA_INT_CLEAR_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_stop_low =
			(void *)HDMA_MSI_STOP_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_stop_high =
			(void *)HDMA_MSI_STOP_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_watermark_low =
			(void *)HDMA_MSI_WATERMARK_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_watermark_high =
			(void *)HDMA_MSI_WATERMARK_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_abort_low =
			(void *)HDMA_MSI_ABORT_LOW_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_abort_high =
			(void *)HDMA_MSI_ABORT_HIGH_OFF_RDCH(cfg_base, channel);
		tmp_ctx->ctx.msi_msgd =
			(void *)HDMA_MSI_MSGD_OFF_RDCH(cfg_base, channel);
	}

	tmp_ctx->type = rw_type;
	tmp_ctx->dpu_dev = dpu_dev;
	tmp_ctx->desc_buf.phy_addr = dpu_dev->bar[DESC_BAR].paddr +
				     HDMA_DESC_SOC_OFFSET_ADDR +
				     channel * 0x1000;
	tmp_ctx->desc_buf.virt_addr = dpu_dev->bar[DESC_BAR].vaddr +
				      HDMA_DESC_SOC_OFFSET_ADDR +
				      channel * 0x1000;
	tmp_ctx->desc_buf.start_pointer_addr = IRAM_BASE_SOC_ADDR +
					       HDMA_DESC_SOC_OFFSET_ADDR +
					       channel * 0x1000;
	tmp_ctx->desc_buf.size = 0x1000;

	atomic_set(&tmp_ctx->desc_buf.flag, 0);
	tmp_ctx->flag = 0;
	init_waitqueue_head(&tmp_ctx->wq);
	tmp_ctx->state = TRANSFER_STATE_IDLE;
	spin_lock_init(&tmp_ctx->lock);


	*ctx = tmp_ctx;

	return 0;
}

int hdma_exit(struct hdma_context *ctx)
{
	kfree(ctx);
	return 0;
}

void hdma_enable(struct hdma_context *ctx)
{
	dpu_dbg("%s!!!\n", __func__);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, 1, ctx->ctx.en);
}

void hdma_disable(struct hdma_context *ctx)
{
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, 0, ctx->ctx.en);
}

int hdma_start_transfer(struct hdma_context *ctx, uint32_t bound)
{
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, 1 << 0, ctx->ctx.doorbell);
	return 0;
}

int hdma_terminal_transfer(struct hdma_context *ctx)
{
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, 1 << 1, ctx->ctx.doorbell);
	return 0;
}

int hdma_set_desc_pointer(struct hdma_context *ctx, uint64_t addr)
{
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, addr & BIT32_MASK, ctx->ctx.llp_low);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, (addr >> 32) & BIT32_MASK, ctx->ctx.llp_high);

	return 0;
}

int hdma_set_attr(struct hdma_context *ctx, uint32_t control1, uint32_t pf,
		  uint32_t cycle_off)
{
	//set ll and memory
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, pf, ctx->ctx.elem_pf);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, cycle_off, ctx->ctx.cycle_off);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, control1, ctx->ctx.control1);

	return 0;
}

void set_xfer_data(struct hdma_context *ctx, uint32_t xfer_size, uint64_t src,
		   uint64_t des)
{
	uint32_t data = 0;

	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, xfer_size, ctx->ctx.xfer_size);
	dpu_dbg("xfer_size  %x read back %x\n", xfer_size, eon_readl(ctx->ctx.xfer_size));

	data = src & BIT32_MASK;
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, data, ctx->ctx.sar_low);
	data = (src >> 32) & BIT32_MASK;
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, data, ctx->ctx.sar_high);

	data = des & BIT32_MASK;
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, data, ctx->ctx.dar_low);
	data = (des >> 32) & BIT32_MASK;
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, data, ctx->ctx.dar_high);
}

uint32_t hdma_get_intr_status(struct hdma_context *ctx)
{
	uint32_t status;

	//dpu_dbg("xfer_size  %x read back %x\n", 0, eon_readl(ctx->ctx.xfer_size));

	status = eon_readl(ctx->ctx.status);
	dpu_dbg("hdma channel status %x\n", status);
	status = eon_readl(ctx->ctx.int_status);
	if (((status)&0x1 == 1) || ((status >> 1) & 0x1 == 1))
	{
		dpu_dbg("hdma err: 0x%x abort 0x%x watermark: 0x%x stop: 0x%x\n",
		(status >> 3) & 0xf, (status >> 2) & 0x1, (status >> 1) & 0x1,
		(status)&0x1);
	}
	return status;
}

void hdma_clear_intr_status(struct hdma_context *ctx, uint32_t val)
{
	if (val == 0) {
		dpu_dbg("hdma clear status all\n");
		eon_writel(7, ctx->ctx.int_clear);
		return;
	}

	dpu_dbg("hdma clear status %x\n", val);
	eon_writel(val, ctx->ctx.int_clear);
	return;
}


void hdma_msi_interrupt(struct hdma_context *ctx, uint32_t msi_addr_h,
			uint32_t msi_addr_l, uint16_t msg_data)
{
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_l, ctx->ctx.msi_stop_low);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_h, ctx->ctx.msi_stop_high);

	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_l, ctx->ctx.msi_abort_low);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_h, ctx->ctx.msi_abort_high);

	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_l, ctx->ctx.msi_watermark_low);
	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msi_addr_h, ctx->ctx.msi_watermark_high);

	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, msg_data, ctx->ctx.msi_msgd);
}

int hdma_enable_intr(struct hdma_context *ctx, uint32_t mask)
{
	uint32_t status = mask;

	if (status == 0) {
		status |= (1 << 6); //local abort
		status |= (1 << 5); //remote abort
		status |= (1 << 4); //locat stop
		status |= (1 << 3); //remote stop
		status |= (1 << 2); //abort mask
		status |= (1 << 1); //watermark mask
		status |= (1 << 0); //mask stop
	}

	hdma_writel(ctx->dpu_dev->base_addr.hdma_base, status, ctx->ctx.int_setup);
	return 0;
}

int hdma_disable_intr(struct hdma_context *ctx)
{
	uint32_t status = 0;

	eon_writel(status, ctx->ctx.int_setup);
	return 0;
}

void dump_desc(struct dma_cmd_desc desc)
{
	dpu_dbg("dump link element:\n");
	dpu_dbg("desc.CB: %x \n", desc.CB);
	dpu_dbg("desc.TCB: %x \n", desc.reserved0);
	dpu_dbg("desc.LLP: %x \n", desc.LLP);
	dpu_dbg("desc.LIE: %d \n", desc.LIE);
	dpu_dbg("desc.RIE: %d \n", desc.RIE);
	dpu_dbg("desc.trasnfer size: %x \n", desc.transfer_size);
	dpu_dbg("desc.sar low: %x \n", desc.sar_low);
	dpu_dbg("desc.sar high: %x \n", desc.sar_high);
	dpu_dbg("desc.dar low: %x \n", desc.dar_low);
	dpu_dbg("desc.dar high: %x \n\n", desc.dar_high);
}
