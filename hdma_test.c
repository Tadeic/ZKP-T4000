#include <linux/delay.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include "hdma.h"
#include <linux/delay.h>

#define TEST_SIZE (1024)

int alloc_test_context(struct hdma_test_context *ctx, struct pci_dev *pdev,
		       dma_addr_t device_dst_addr, uint32_t size)
{
	ctx->size = size;
	ctx->device_dst = device_dst_addr;
	ctx->srcsv =
		dma_alloc_coherent(&pdev->dev, size, &ctx->srcs, GFP_KERNEL);
	if (ctx->srcsv == NULL) {
		pr_err("failed to alloc srcv\n");
		return -ENOMEM;
	}

	ctx->dstsv =
		dma_alloc_coherent(&pdev->dev, size, &ctx->dsts, GFP_KERNEL);
	if (ctx->dstsv == NULL) {
		dma_free_coherent(&pdev->dev, ctx->size, ctx->srcsv, ctx->srcs);
		pr_err("failed to alloc dstsv\n");
		return -ENOMEM;
	}

	ctx->link_elements = kmalloc(0x1000, GFP_KERNEL);
	if (ctx->link_elements == NULL) {
		dma_free_coherent(&pdev->dev, ctx->size, ctx->dstsv, ctx->dsts);
		dma_free_coherent(&pdev->dev, ctx->size, ctx->srcsv, ctx->srcs);
		pr_err("failed to alloc srcv\n");
		return -ENOMEM;
	}

	dpu_dbg("dma addr src %llx des %llx\n", ctx->srcs, ctx->dsts);
	get_random_bytes(ctx->srcsv, size);
	//memset(ctx->srcsv, 0x11, size);
	//get_random_bytes(ctx->dstsv, size);
	//memset(ctx->dstsv, 0x11, size);
	//pr_err("random buf[0] 0x%x\n", ((uint32_t *)ctx->srcsv)[0]);
	ctx->starttime = 0;
	ctx->end = 0;
	memset(ctx->name, 0, sizeof(ctx->name));
	atomic_set(&ctx->flag, 1);

	return 0;
}

void free_test_context(struct hdma_test_context *ctx, struct pci_dev *pdev)
{
	kfree(ctx->link_elements);
	dma_free_coherent(&pdev->dev, ctx->size, ctx->srcsv, ctx->srcs);
	dma_free_coherent(&pdev->dev, ctx->size, ctx->dstsv, ctx->dsts);
}

void record_start_time(struct hdma_test_context *test_ctx)
{
	test_ctx->starttime = ktime_get();
	dpu_dbg("starttime %lld usecs\n", test_ctx->starttime);
}

void record_end_time(struct hdma_test_context *test_ctx)
{
	test_ctx->end = ktime_get();
	dpu_dbg("end %lld usecs\n", test_ctx->end);
}

unsigned long long get_run_time(struct hdma_test_context *test_ctx)
{
	test_ctx->delta = ktime_sub(test_ctx->end, test_ctx->starttime);
	dpu_dbg("test used %lld usecs\n", test_ctx->delta);
	test_ctx->duration =
		(unsigned long long)ktime_to_ns(test_ctx->delta) >> 10;
	dpu_dbg("test used %lld usecs\n", test_ctx->duration);
	return test_ctx->duration;
}

int dma_single_inbound_test(struct hdma_context *ctx,
			    struct hdma_test_context *test_ctx)
{
	uint32_t val;
	struct dma_cmd_desc *pdesc;
	struct eon_device *eon_dev = ctx->dpu_dev;
	int rv = 0;

	void *descv = ctx->desc_buf.virt_addr;
	dpu_dbg("%s start!!!\n", __func__);

	pdesc = descv;

	spin_lock(&ctx->lock);
	ctx->data = (void *)test_ctx;
	sprintf(test_ctx->name, "%s", __func__);
	record_start_time(test_ctx);

	//hdma_select_write(ctx, 0);
	//hdma_terminal_transfer(ctx);
	//hdma_clear_intr_status(ctx, 0);
	//hdma_set_attr(ctx, 0);

	//hdma_enable(ctx);
	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status before dma start:%x\n", val);

	// hdma_msi_interrupt(ctx, eon_dev->base_addr.msi_addr_h,
	// 		   eon_dev->base_addr.msi_addr_l,
	// 		   eon_dev->base_addr.msg_data);
	hdma_set_attr(ctx, 0xd0000, 0, 0);
	hdma_enable_intr(ctx, 0x3f);

	dpu_dbg("xfer start from %llx to %llx size %x\n", test_ctx->srcs,
		test_ctx->device_dst, test_ctx->size);
	set_xfer_data(ctx, test_ctx->size, test_ctx->srcs,
		      test_ctx->device_dst);
	ctx->state = TRANSFER_STATE_RUNNING;
	hdma_start_transfer(ctx, 0);

	rv = wait_event_interruptible_timeout(
		ctx->wq, (ctx->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(1000));

	if (rv == 0) {
		dpu_err("hdma time out. rd %d wr %d \n", ctx->state, ctx->state);
	}

	spin_unlock(&ctx->lock);
	record_end_time(test_ctx);
	dpu_dbg("%s after %lld usecs\n", test_ctx->name,
		get_run_time(test_ctx));
	atomic_set(&test_ctx->flag, 0);

	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status %x\n", val);
	hdma_clear_intr_status(ctx, val);
	//hdma_terminal_transfer(ctx);

	//hdma_disable(ctx);
	return 0;
}

int dma_single_inbound_llmode_test(struct hdma_context *ctx,
				   struct hdma_test_context *test_ctx)
{
	//struct eon_device *eon_dev = ctx->dpu_dev;
	struct dma_cmd_desc *pdesc;
	uint32_t val;
	int rv = 0;

	void *descv = ctx->desc_buf.virt_addr;
	uint64_t start_addr = ctx->desc_buf.start_pointer_addr; //64k start
	dpu_dbg("%s %d\n", __func__, __LINE__);

	//pdesc = test_ctx->link_elements;
	pdesc = descv;
	spin_lock(&ctx->lock);

	ctx->data = (void *)test_ctx;
	sprintf(test_ctx->name, "%s", __func__);
	record_start_time(test_ctx);

	//hdma_select_write(ctx, 0);

	dpu_dbg("Prepare link list data:\n");

	set_data_element(pdesc, DMA_MEM_TO_DEV, test_ctx->srcs,
			 test_ctx->device_dst, test_ctx->size, 1, 1, 1);
	set_link_element((struct end_element *)(pdesc + 1), start_addr, 1, 0);
	// mdelay(100);

	// hdma_enable(ctx);
	// mdelay(1);
	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status before dma start:%x\n", val);

	// hdma_msi_interrupt(ctx, eon_dev->base_addr.msi_addr_h,
	// 		   eon_dev->base_addr.msi_addr_l,
	// 		   eon_dev->base_addr.msg_data);
	hdma_set_attr(ctx, 0xe003, 0, 2);
	hdma_enable_intr(ctx, 0x3f);

	dpu_dbg("hdma inter status before dma start:%x\n", val);
	hdma_set_desc_pointer(ctx, start_addr);

	ctx->state = TRANSFER_STATE_RUNNING;
	hdma_start_transfer(ctx, 0);
	dpu_dbg("s 0x%x\n", ((char *)test_ctx->srcsv)[10]);

	rv = wait_event_interruptible_timeout(
		ctx->wq, (ctx->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(1000));
	// mdelay(100);
	spin_unlock(&ctx->lock);
	record_end_time(test_ctx);
	dpu_dbg("%s after %lld usecs\n", test_ctx->name,
		get_run_time(test_ctx));
	atomic_set(&test_ctx->flag, 0);

	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status %x\n", val);
	hdma_clear_intr_status(ctx, val);
	hdma_terminal_transfer(ctx);

	//hdma_disable(ctx);
	dpu_dbg("version--- %x \n", val);

	return 0;
}

int dma_single_outbound_test(struct hdma_context *ctx,
			     struct hdma_test_context *test_ctx)
{
	struct dma_cmd_desc *pdesc;
	//struct eon_device *eon_dev = ctx->dpu_dev;
	int rv = 0;
	uint32_t val;

	void *descv = ctx->desc_buf.virt_addr;
	dpu_dbg("%s start!!!\n", __func__);

	pdesc = descv;
	spin_lock(&ctx->lock);

	ctx->data = (void *)test_ctx;
	sprintf(test_ctx->name, "%s", __func__);
	record_start_time(test_ctx);

	//hdma_select_write(ctx, 1);

	// hdma_enable(ctx);
	//hdma_terminal_transfer(ctx);
	// mdelay(1);
	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status before dma start:%x\n", val);

	// hdma_msi_interrupt(ctx, eon_dev->base_addr.msi_addr_h,
	// 		   eon_dev->base_addr.msi_addr_l,
	// 		   eon_dev->base_addr.msg_data);
	hdma_set_attr(ctx, 0x70002, 0, 0);
	hdma_enable_intr(ctx, 0x3f);

	set_xfer_data(ctx, test_ctx->size, test_ctx->device_dst,
		      test_ctx->dsts);
	ctx->state = TRANSFER_STATE_RUNNING;
	hdma_start_transfer(ctx, 0);

	rv = wait_event_interruptible_timeout(
		ctx->wq, (ctx->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(1000));

	spin_unlock(&ctx->lock);
	record_end_time(test_ctx);

	if (rv == 0) {
		dpu_err("hdma time out. rd %d wr %d \n", ctx->state, ctx->state);
	}


	dpu_dbg("%s after %lld usecs\n", test_ctx->name,
		get_run_time(test_ctx));
	atomic_set(&test_ctx->flag, 0);

	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status %x\n", val);
	hdma_clear_intr_status(ctx, val);

	return 0;
}

int dma_single_outbound_llmode_test(struct hdma_context *ctx,
				    struct hdma_test_context *test_ctx)
{
	struct dma_cmd_desc *pdesc;
	//struct eon_device *eon_dev = ctx->dpu_dev;
	int rv = 0;
	uint32_t val;

	void *descv = ctx->desc_buf.virt_addr;
	uint64_t start_addr = ctx->desc_buf.start_pointer_addr;

	pdesc = descv;
	dpu_dbg("%s lock \n", __func__);
	spin_lock(&ctx->lock);

	ctx->data = (void *)test_ctx;
	sprintf(test_ctx->name, "%s", __func__);
	record_start_time(test_ctx);

	//hdma_select_write(ctx, 1);
	// mdelay(1);

	set_data_element(pdesc, DMA_DEV_TO_MEM, test_ctx->device_dst,
			 test_ctx->dsts, test_ctx->size, 1, 1, 1);
	//set_data_element(pdesc, DMA_DEV_TO_MEM, test_ctx->dsts, test_ctx->device_dst, test_ctx->size, 1, 0, 1);
	set_link_element((struct end_element *)(pdesc + 1), start_addr, 1, 0);

	//hdma_terminal_transfer(ctx);
	// hdma_enable(ctx);
	// mdelay(1);
	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status before dma start:%x\n", val);

	// hdma_msi_interrupt(ctx, eon_dev->base_addr.msi_addr_h,
	// 		   eon_dev->base_addr.msi_addr_l,
	// 		   eon_dev->base_addr.msg_data);
	hdma_set_attr(ctx, 3, 0, 2);
	hdma_enable_intr(ctx, 0x3f);

	hdma_set_desc_pointer(ctx, start_addr);

	dump_desc(*pdesc);

	ctx->state = TRANSFER_STATE_RUNNING;
	hdma_start_transfer(ctx, 1);

	rv = wait_event_interruptible_timeout(
		ctx->wq, (ctx->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(1000));

	spin_unlock(&ctx->lock);
	record_end_time(test_ctx);
	dpu_dbg("%s after %lld usecs\n", test_ctx->name,
		get_run_time(test_ctx));
	atomic_set(&test_ctx->flag, 0);

	val = hdma_get_intr_status(ctx);
	dpu_dbg("hdma inter status %x\n", val);

	//hdma_disable(ctx);

	dpu_dbg("version--- %x \n", val);

	return 0;
}

void dump_mem(void __iomem *addr, uint32_t byte_size)
{
	uint32_t i = 0;
	uint8_t *ptr = (uint8_t *)addr;

	dpu_dbg("dump start from:%llx\n", (uint64_t)addr);
	for (i = 0; i < byte_size; i++) {
		printk(KERN_CONT "0x%02x ", ptr[i]);
		if ((i % 16) == 15)
			printk(KERN_ERR "\n");
	}
	dpu_dbg("dump  end!\n");
}

void test_result_check(struct hdma_test_context test_ctx)
{
	uint32_t i = 0;
	uint32_t counter = 0;

	for (i = 0; i < test_ctx.size; i++) {
		if (((char *)test_ctx.srcsv)[i] != ((char *)test_ctx.dstsv)[i])
			counter++;
	}

	if (counter != 0) {
		dpu_dbg("data copy error count: %d\n", counter);
	} else {
		dpu_dbg("data copy check OK!!!\n");
	}

	if (memcmp(test_ctx.srcsv, test_ctx.dstsv, test_ctx.size) != 0) {
		dpu_dbg("0x%x\n", ((char *)test_ctx.dstsv)[10]);
		dpu_dbg("failed to copy dstsv\n");
	} else {
		dpu_dbg("0x%x\n", ((char *)test_ctx.dstsv)[10]);
		dpu_dbg("data copy OK!!!\n");
	}
}



// {
// 	eon_writel(1, ctx->used_ctx->en);
// 	eon_writel(0, ctx->used_ctx->elem_pf);
// 	eon_writel(0, ctx->used_ctx->cycle_off);
// 	eon_writel(0xd0000, ctx->used_ctx->control1);

// 	set_xfer_data(ctx, test_ctx->size, test_ctx->device_dst,
// 		      test_ctx->dsts);
// 	eon_writel(1 << 0, ctx->used_ctx->doorbell);

// }

