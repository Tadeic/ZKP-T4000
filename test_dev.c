#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/dmaengine.h>
#include <asm/cacheflush.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include "test_dev.h"
#include "hdma.h"
#include "iatu.h"

extern struct class *eon_class;
extern unsigned int eon_dpu_major;

static int device_open(struct inode *inode, struct file *filp)
{
	struct char_dev_ctx *cdev;
	struct eon_device *dpu_dev;
	int index;

	dpu_dbg("%s ++ \n", __func__);
	index = iminor(inode);
	cdev = container_of(inode->i_cdev, struct char_dev_ctx, dev);
	//BUG_ON(cdev->magic != 0x55);
	dpu_dev = cdev->dpu_dev;
	filp->private_data = dpu_dev;

	return 0;
}

static int device_release(struct inode *inode, struct file *filp)
{
	struct eon_device *dpu_dev;
	dpu_dbg("%s ++ \n", __func__);
	dpu_dev = (struct eon_device *)filp->private_data;
	BUG_ON(!dpu_dev);

	//todo add dma stop

	return 0;
}

void sgt_dump(struct sg_table *sgt)
{
	int i;
	struct scatterlist *sg = sgt->sgl;

	pr_info("sgt 0x%p, sgl 0x%p, nents %u/%u.\n", sgt, sgt->sgl, sgt->nents,
		sgt->orig_nents);

	for (i = 0; i < sgt->orig_nents; i++, sg = sg_next(sg))
		pr_info("%d, 0x%p, pg 0x%p,%u+%u, dma 0x%llx,%u.\n", i, sg,
			sg_page(sg), sg->offset, sg->length, sg_dma_address(sg),
			sg_dma_len(sg));
}

static uint64_t get_pages_nr(char __user *buf, unsigned long len)
{
	return (((unsigned long)buf + len + PAGE_SIZE - 1) -
		((unsigned long)buf & PAGE_MASK)) >>
	       PAGE_SHIFT;
}

int dpu_get_pages(uint64_t buf, uint64_t len, struct dpu_host_mem_info *info)
{
	int ret = 0;
	int i, flag = 0;
	struct dpu_page_blocks *blocks_tmp;
	unsigned long blocks_nr, blocks_nr_tmp;
	struct page **pages_tmp;

	if (!info) {
		pr_err("err --%s %d \n", __func__, __LINE__);
		return -1;
	}

	info->addr = buf;
	blocks_nr = get_pages_nr((char *)buf, len);
	blocks_nr_tmp = blocks_nr;
	blocks_tmp = vmalloc(blocks_nr * sizeof(struct dpu_page_blocks));
	if (!blocks_tmp) {
		ret = -1;
		pr_err("err --%s %d \n", __func__, __LINE__);
		return ret;
	}
	memset(blocks_tmp, 0, blocks_nr_tmp * sizeof(struct dpu_page_blocks));

	info->blocks_nr = blocks_nr;
	pages_tmp = vmalloc(info->blocks_nr * sizeof(struct page *));
	if (!pages_tmp) {
		ret = -2;
		vfree(blocks_tmp);
		pr_err("err --%s %d \n", __func__, __LINE__);
		return ret;
	}
	flag = FOLL_WRITE; // need to check
	ret = get_user_pages_fast((unsigned long)buf, info->blocks_nr, flag,
				  pages_tmp);
	if (ret < info->blocks_nr) {
		pr_err("failed to get user page\n");
		if (ret > 0) {
			for (i = 0; pages_tmp && i < ret; i++)
				if (pages_tmp[i])
					put_page(pages_tmp[i]);
		}
		vfree(pages_tmp);
		vfree(blocks_tmp);
		ret = -3;
		return ret;
	}

	info->blocks = blocks_tmp;
	for (i = 0; i < blocks_nr; i++) {
		blocks_tmp = info->blocks + i;
		blocks_tmp->pages = pages_tmp[i];
		blocks_tmp->pages_nr = 1;
	}

	vfree(pages_tmp);

	return 0;
}

int dpu_put_pages(struct dpu_host_mem_info *info)
{
	struct dpu_page_blocks *blocks;
	int ret = 0;
	int i = 0;

	if (!info)
		return -1;

	for (i = 0; info->blocks && i < info->blocks_nr; i++) {
		blocks = info->blocks + i;
		if (blocks->pages)
			put_page(blocks->pages);
	}

	vfree(info->blocks);

	return ret;
}

// fill sg table
static int dpu_setup_sg(struct dpu_xfer_info *xfer_info, struct pci_dev *pdev,
			uint64_t host_addr, uint32_t in_out_bound)
{
	uint64_t first_sg_len = 0, last_sg_len = 0;
	struct dpu_host_mem_info *hmem_info;
	uint64_t count = xfer_info->size;
	struct dpu_page_blocks *block;
	struct scatterlist *sg = NULL;
	struct sg_table *sgt = NULL;
	int sg_nr, page_nr;
	int i, j, ret = 0;

	dpu_dbg("arg size %llx\n", xfer_info->size);

	//get page
	hmem_info = &xfer_info->hmem_info;
	ret = dpu_get_pages(host_addr, count, hmem_info);
	if (ret) {
		dpu_err("failed to get pages ret = %d\n", ret);
		return ret;
	}

	//flush d-cache
	sg_nr = hmem_info->blocks_nr;
	for (i = 0; i < sg_nr; i++) {
		block = hmem_info->blocks + i;
		page_nr = block->pages_nr;
		for (j = 0; j < page_nr; j++) {
			flush_dcache_page(block->pages + j);
		}
	}

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		dpu_err("failed to alloc sgt\n");
		ret = -1;
		goto err_alloc_sgt;
	}

	ret = sg_alloc_table(sgt, sg_nr, GFP_KERNEL);
	if (ret) {
		dpu_err("failed to alloc sg table\n");
		ret = -2;
		goto err_alloc_table;
	}

	for_each_sg (sgt->sgl, sg, sg_nr, i) {
		struct page *page = NULL;
		unsigned int size, nbytes;
		unsigned int offset = offset_in_page(host_addr);
		block = hmem_info->blocks + i;

		page = block->pages;
		size = block->pages_nr * PAGE_SIZE - offset;
		nbytes = min_t(unsigned int, size, count);
		dpu_dbg("size %d offert %d, count %lld ------\n", size, offset,
			count);
		sg_set_page(sg, page, nbytes, offset);
		if (i == 0)
			first_sg_len = nbytes;
		if (i == sg_nr - 1)
			last_sg_len = nbytes;
		host_addr += nbytes;
		count -= nbytes;
	}

	// map_to dma
	if (!pci_map_sg(pdev, sgt->sgl, sg_nr, xfer_info->dir)) {
		dpu_err("failed to map sg\n");
		ret = -3;
		goto err_map_sg;
	}

	dpu_dbg("----------sg num %d\n", sg_nr);
	//dump_sgt(sgt);

	xfer_info->sgt = sgt;
	xfer_info->sglen = sg_nr;

	//xfer_info->align.src = src;
	//xfer_info->align.des = des;
	xfer_info->align.size = xfer_info->size;

	dpu_dbg("align size %llx first_sg_len %llx\n", xfer_info->align.size,
		first_sg_len);
	// if (xfer_info->align.size) {
	// 		xfer_info->align.sg_start = 0;
	// 		xfer_info->align.sg_end = sg_nr - 1;
	// }

	// dpu_dbg("align:sg_start = %d sg_end = %d\n", xfer_info->align.sg_start,
	// 	xfer_info->align.sg_end);

	return 0;

err_map_sg:
	sg_free_table(sgt);
err_alloc_table:
	kfree(sgt);
err_alloc_sgt:
	dpu_put_pages(hmem_info);

	return ret;
}

static void dpu_teardown_sg(struct dpu_xfer_info *xfer_info)
{
	sg_free_table(xfer_info->sgt);
	kfree(xfer_info->sgt);
	dpu_put_pages(&xfer_info->hmem_info);
}

static int set_hdma_desc(struct dpu_xfer_info *xfer_info,
			 struct dpu_desc_info *desc_info, struct pci_dev *pdev)
{
	uint64_t descs_size, host_addr, src_addr, des_addr, len,
		offset = 0, xfer_len = 0, desc_addr;
	struct dma_cmd_desc *descs_host_addr;
	struct scatterlist *sg;
	uint32_t i;

	descs_size = ((xfer_info->sglen + 1) * sizeof(struct dma_cmd_desc));
	dpu_dbg("%s %d 0x%llx %x++", __func__, __LINE__, descs_size,
		xfer_info->sglen);

	descs_host_addr = kmalloc(descs_size, GFP_KERNEL);
	if (!descs_host_addr) {
		dpu_err("%s %d error\n", __func__, __LINE__);
		return -1;
	}
	desc_info->desc_host_addr = (uint64_t)descs_host_addr;
	desc_addr = pci_map_single(pdev, descs_host_addr, descs_size,
				   desc_info->dir);
	if (unlikely(pci_dma_mapping_error(pdev, desc_addr))) {
		kfree(descs_host_addr);
		dpu_err("%s %d map single\n", __func__, __LINE__);
		return -1;
	}

	for_each_sg (xfer_info->sgt->sgl, sg, xfer_info->sglen, i) {
		len = sg_dma_len(sg);

		host_addr = sg_dma_address(sg) + offset;
		if (xfer_info->dir == DMA_MEM_TO_DEV) {
			des_addr = xfer_info->align.dst + xfer_len;
			src_addr = host_addr;
			//dpu_dbg("---from %llx to %llx \n", host_addr, des_addr);
		} else {
			src_addr = xfer_info->align.src + xfer_len;
			des_addr = host_addr;
		}

		dpu_dbg("%llx %llx %llx++", src_addr, des_addr, len);
		if (i == (xfer_info->sglen - 1)) {
			set_data_element(descs_host_addr, xfer_info->dir,
					 src_addr, des_addr, len, 1, 1, 1);
		} else {
			set_data_element(descs_host_addr, xfer_info->dir,
					 src_addr, des_addr, len, 0, 0, 1);
		}
		dump_desc(*descs_host_addr);
		descs_host_addr++;
		xfer_len += len;
		offset = 0;
	}
	set_link_element((struct end_element *)descs_host_addr,
			 desc_info->desc_device_addr, 1, 0); //todo check

	desc_info->desc_size = descs_size;
	desc_info->desc_num = xfer_info->sglen;
	desc_info->desc_addr = desc_addr;

	return 0;
}

static void teardown_dma_desc(struct pci_dev *pdev,
			      struct dpu_desc_info *desc_info)
{
	pci_unmap_single(pdev, desc_info->desc_addr, desc_info->desc_size,
			 desc_info->dir);
	kfree((void *)desc_info->desc_host_addr);
}

static int tranfer_desc(struct hdma_context *hdma_dev,
			struct dpu_desc_info *desc_info)
{
	//struct eon_device *eon_dev = hdma_dev->dpu_dev;
	//uint32_t val;
	int rv = 0, ret = 0;

	dpu_dbg("%s start!!!\n", __func__);

	spin_lock(&hdma_dev->lock);
	//hdma_select_write(hdma_dev, 0);
	
	//hdma_enable(hdma_dev);
	
	// hdma_msi_interrupt(hdma_dev, eon_dev->base_addr.msi_addr_h,
	// 		   eon_dev->base_addr.msi_addr_l,
	// 		   eon_dev->base_addr.msg_data);
	hdma_set_attr(hdma_dev, 0xd0000, 0, 0);
	hdma_enable_intr(hdma_dev, 0x3d);

	dpu_dbg("xfer start from %llx to %llx size %llx\n",
		desc_info->desc_addr, desc_info->desc_device_addr,
		desc_info->desc_size);
	set_xfer_data(hdma_dev, desc_info->desc_size, desc_info->desc_addr,
		      desc_info->desc_device_addr);
	hdma_dev->state = TRANSFER_STATE_RUNNING;
	hdma_start_transfer(hdma_dev, 0);
#ifdef ENABLE_HDMA_TIMEOUT
	rv = wait_event_interruptible_timeout(
		hdma_dev->wq, (hdma_dev->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(HDMA_TIMEOUT_LENGTH));

	if (rv == 0) {
		dpu_err("hdma transfer time out %d.\n", HDMA_TIMEOUT_LENGTH);
		ret = -1;
	}
#else
	rv = wait_event_interruptible(hdma_dev->wq, (hdma_dev->state !=
						     TRANSFER_STATE_RUNNING));
	ret = rv;
#endif
	spin_unlock(&hdma_dev->lock);

	// if (rv == 0) {
	// 	dpu_err("hdma transfer time out.\n");
	// }

	//mdelay(100);

	// val = hdma_get_intr_status(hdma_dev);
	// dpu_dbg("hdma inter status %x\n", val);
	// hdma_clear_intr_status(hdma_dev, val);

	return ret;
}

static int regs_operations(struct eon_device *dpu_dev,
			   struct dpu_reg_args *regs_args)
{
	// int ret = 0;
	uint64_t offset = 0, map_addr = 0;

	if (regs_args->reg_addr > REGS_ADDR_MAX ||
	    regs_args->reg_addr < REGS_ADDR_MIN) {
		dpu_err("Error reg addr %llx\n", regs_args->reg_addr);
		return -1;
	}

	mutex_lock(&dpu_dev->conbar_lock);
	offset = regs_args->reg_addr & (CONFIG_BAR_SIZE - 1);
	map_addr = regs_args->reg_addr - offset;
	if (map_addr != dpu_dev->config_bar_base) {
		iatu_map_inbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1, map_addr,
				     CONFIG_BAR_SIZE);
		iatu_map_outbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1, map_addr,
				      CONFIG_BAR_SIZE);
		dpu_dev->config_bar_base = map_addr;
	}

	dpu_dbg("regs addr+++ %llx data %x\n", regs_args->reg_addr,
		regs_args->reg_data);
	switch (regs_args->rw_flag) {
	case REG_WRITE:
		eon_writel(regs_args->reg_data,
			   offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	case REG_READ:
		regs_args->reg_data = eon_readl(
			offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	default:
		dpu_err("Unknow regs operation\n");
		break;
	}
	dpu_dbg("regs addr--- %llx data %x\n", regs_args->reg_addr,
		regs_args->reg_data);

	mutex_unlock(&dpu_dev->conbar_lock);
	return 0;
}

static int dma_llmode_transmit(struct hdma_context *hdma_dev,
			       struct dpu_desc_info *desc_info, uint32_t enable,
			       struct reg_infos *reg_infos)
{
	//struct eon_device *eon_dev = hdma_dev->dpu_dev;
	uint32_t value;
	size_t i;
	int rv = 0, ret = 0;

	//pdesc = test_ctx->link_elements;
	//dpu_dbg("Dma inbound start!! \n");
	spin_lock(&hdma_dev->lock);
	dpu_dbg("Dma start !!\n");

	hdma_set_attr(hdma_dev, 0x003, 0, 2);
	//hdma_enable_intr(hdma_dev, 0x3d);
	hdma_enable_intr(hdma_dev, 0x18);

	dpu_dbg("hdma inter status before dma start:%x\n", value);
	hdma_set_desc_pointer(hdma_dev, desc_info->desc_device_addr);

	if (desc_info->dir == DMA_MEM_TO_DEV) {
		hdma_dev->state = TRANSFER_STATE_RUNNING;
	} else {
		hdma_dev->state = TRANSFER_STATE_RUNNING;
	}

	if (reg_infos->reg_operation == reg_enable) {
		if (reg_infos->num < REG_OPERATION_MAX_NUM) {
			for (i = 0; i < reg_infos->num; i++) {
				struct dpu_reg_args regs_args;
				regs_args.reg_addr =
					reg_infos->write_data[i].reg_addr;
				regs_args.reg_data =
					reg_infos->write_data[i].reg_data;
				regs_args.rw_flag = REG_WRITE;
				rv = regs_operations(hdma_dev->dpu_dev,
						     &regs_args);
				if (rv) {
					spin_unlock(&hdma_dev->lock);
					return rv;
				}
			}
		}
	}

	if (enable == dma_enable) {
		hdma_start_transfer(hdma_dev, 0);
	}

#ifdef ENABLE_HDMA_TIMEOUT
	rv = wait_event_interruptible_timeout(
		hdma_dev->wq, (hdma_dev->state != TRANSFER_STATE_RUNNING),
		msecs_to_jiffies(HDMA_TIMEOUT_LENGTH));
	if (rv == 0) {		
		dpu_err("wait_event_interruptible_timeout %d.\n", HDMA_TIMEOUT_LENGTH);
		ret = -1;
	}
#else
	rv = wait_event_interruptible(hdma_dev->wq, (hdma_dev->state !=
						     TRANSFER_STATE_RUNNING));
	ret = rv;
#endif
	spin_unlock(&hdma_dev->lock);

	return ret;
}

static ssize_t device_read(struct file *filp, char __user *buf, size_t count,
			   loff_t *ppos)
{
	return 0;
}

static ssize_t device_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	return 0;
}

static int eon_hdma_inbound(struct hdma_context *hdma_dev, uint64_t host_src,
			    uint64_t device_des, uint64_t len,
			    uint64_t desc_addr, uint32_t enable,
			    struct reg_infos *reg_infos)
{
	struct dpu_xfer_info xfer_info = { 0 };
	struct dpu_desc_info desc_info = { 0 };
	// size_t tc = 0;
	int ret = 0;

	xfer_info.align.src = host_src;
	xfer_info.align.dst = device_des;
	xfer_info.size = len;
	xfer_info.dir = DMA_MEM_TO_DEV;
	//xfer_info.align.desc_addr = desc_addr;
	ret = dpu_setup_sg(&xfer_info, hdma_dev->dpu_dev->pdev, host_src, 1);
	if (ret) {
		dpu_dbg("dpu setup sg error\n");
		return ret;
	}

	dpu_dbg("transmit data for %llx to %llx \n", xfer_info.align.src,
		xfer_info.align.dst);
	desc_info.dir = DMA_MEM_TO_DEV;
	desc_info.desc_device_addr = desc_addr;
	ret = set_hdma_desc(&xfer_info, &desc_info, hdma_dev->dpu_dev->pdev);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_set_dma;
	}

	// mdelay(10);
	dpu_dbg("start transmit desc---!\n");
	ret = tranfer_desc(hdma_dev, &desc_info);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_tranfer_desc;
	}

	dpu_dbg("===>start transmit data!!!!!\n");
	// mdelay(10);
	ret = dma_llmode_transmit(hdma_dev, &desc_info, enable, reg_infos);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_dma_llmode;
	}
	teardown_dma_desc(hdma_dev->dpu_dev->pdev, &desc_info);
	dpu_teardown_sg(&xfer_info);

	dpu_dbg("<====end transmit data!!!!!\n");
	return ret;
err_dma_llmode:
err_tranfer_desc:
	teardown_dma_desc(hdma_dev->dpu_dev->pdev, &desc_info);
err_set_dma:
	dpu_teardown_sg(&xfer_info);
	return ret;
}

static int eon_hdma_outbound(struct hdma_context *hdma_dev,
			     struct hdma_context *hdma_dev_desc,
			     uint64_t host_des, uint64_t device_src,
			     uint64_t len, uint64_t desc_addr, uint32_t enable,
			     struct reg_infos *reg_infos)
{
	struct dpu_xfer_info xfer_info = { 0 };
	struct dpu_desc_info desc_info = { 0 };
	size_t tc = 0;
	int ret = 0;

	xfer_info.align.src = device_src;
	xfer_info.align.dst = host_des;
	xfer_info.size = len;
	xfer_info.dir = DMA_DEV_TO_MEM;
	//xfer_info.align.desc_addr = desc_addr;
	ret = dpu_setup_sg(&xfer_info, hdma_dev->dpu_dev->pdev, host_des, 1);
	if (ret) {
		dpu_dbg("dpu setup sg error\n");
		return ret;
	}

	dpu_dbg("transmit data for %llx to %llx \n", xfer_info.align.src,
		xfer_info.align.dst);
	desc_info.dir = DMA_DEV_TO_MEM;
	desc_info.desc_device_addr = desc_addr;
	ret = set_hdma_desc(&xfer_info, &desc_info, hdma_dev->dpu_dev->pdev);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_set_dma;
	}
	dpu_dbg("start transmit desc!\n");
	// mdelay(10);
	ret = tranfer_desc(hdma_dev_desc, &desc_info);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_tranfer_desc;
	}

	dpu_dbg("start transmit data!!\n");
	// mdelay(10);
	ret = dma_llmode_transmit(hdma_dev, &desc_info, enable, reg_infos);
	if (ret) {
		dpu_dbg("%s %d error\n", __func__, __LINE__);
		goto err_dma_llmode;
	}
	teardown_dma_desc(hdma_dev->dpu_dev->pdev, &desc_info);
	dpu_teardown_sg(&xfer_info);

	return ret;
err_dma_llmode:
err_tranfer_desc:
	teardown_dma_desc(hdma_dev->dpu_dev->pdev, &desc_info);
err_set_dma:
	dpu_teardown_sg(&xfer_info);
	return tc;
}

static void dump_dma_args(struct hdma_mem_args *hdma_args)
{
	dpu_dbg("dma: %d\n", hdma_args->hdma_id);
	dpu_dbg("host addr: 0x%llx device addr: 0x%llx link list addr: 0x%llx size %llx\n",
		hdma_args->host_addr, hdma_args->device_addr,
		hdma_args->desc_addr, hdma_args->mem_size);
}

static int eon_hdma(struct eon_device *dpu_dev, unsigned int cmd,
		    unsigned long args)
{
	struct hdma_context *hdma_dev;
	struct hdma_mem_args *hdma_args;
	int ret = 0;

	hdma_args = kmalloc(sizeof(struct hdma_mem_args), GFP_KERNEL);
	if (!hdma_args) {
		pr_err("failed to alloc cmd args\n");
		return -1;
	}

	if (cmd & IOC_IN) {
		if (copy_from_user(hdma_args, (void __user *)args,
				   sizeof(struct hdma_mem_args))) {
			kfree(hdma_args);
			pr_err("copy from user failed\n");
			return (-2);
		}
	}

	if (hdma_args->hdma_id >= MAX_HDMA_NUMBER) {
		dpu_err("error dma index %d\n", hdma_args->hdma_id);
		return -1;
	}
	switch (hdma_args->op) {
	case COMMAND_HDMA_INBOUND:
		// dpu_err("eon_hdma_inbound hdms id %x\n", hdma_args->hdma_id);
		hdma_dev = dpu_dev->hdmar[hdma_args->hdma_id];
		ret = eon_hdma_inbound(hdma_dev, hdma_args->host_addr,
				       hdma_args->device_addr,
				       hdma_args->mem_size,
				       hdma_args->desc_addr, hdma_args->enable,
				       &hdma_args->regs);
		break;
	case COMMAND_HDMA_OUTBOUND:
		// dpu_err("eon_hdma_outbound hdms id %x\n", hdma_args->hdma_id);
		hdma_dev = dpu_dev->hdmaw[hdma_args->hdma_id];
		ret = eon_hdma_outbound(
			hdma_dev, dpu_dev->hdmar[hdma_args->hdma_id],
			hdma_args->host_addr, hdma_args->device_addr,
			hdma_args->mem_size, hdma_args->desc_addr,
			hdma_args->enable, &hdma_args->regs);
		break;
	default:
		dpu_err("Error unknow hdma op %d\n", hdma_args->op);
		break;
	}
	if (!ret && (cmd & IOC_OUT)) {
		if (copy_to_user((void __user *)args, hdma_args,
				 sizeof(struct hdma_mem_args))) {
			pr_err("failed to copy to user\n");
			ret = -3;
		}
	}

	kfree(hdma_args);

	return ret;
}

static int eon_rw_reg(struct eon_device *dpu_dev, unsigned int cmd,
		      unsigned long args)
{
	struct dpu_reg_args *regs_args;
	int ret = 0;
	uint64_t offset = 0, map_addr = 0;

	regs_args = kmalloc(sizeof(struct dpu_reg_args), GFP_KERNEL);
	if (!regs_args) {
		dpu_err("failed to alloc cmd args\n");
		return -1;
	}

	if (cmd & IOC_IN) {
		if (copy_from_user(regs_args, (void __user *)args,
				   sizeof(struct dpu_reg_args))) {
			kfree(regs_args);
			dpu_err("copy from user failed\n");
			return (-2);
		}
	}

	if (regs_args->reg_addr > REGS_ADDR_MAX ||
	    regs_args->reg_addr < REGS_ADDR_MIN) {
		dpu_err("Error reg addr %llx\n", regs_args->reg_addr);
		return -1;
	}

	mutex_lock(&dpu_dev->conbar_lock);
	offset = regs_args->reg_addr & (CONFIG_BAR_SIZE - 1);
	map_addr = regs_args->reg_addr - offset;
	if (map_addr != dpu_dev->config_bar_base) {
		iatu_map_inbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1, map_addr,
				     CONFIG_BAR_SIZE);
		iatu_map_outbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1, map_addr,
				      CONFIG_BAR_SIZE);
		dpu_dev->config_bar_base = map_addr;
	}
#if 0
	if (regs_args->reg_addr >=
		    (dpu_dev->config_bar_base + CONFIG_BAR_SIZE) ||
	    regs_args->reg_addr < dpu_dev->config_bar_base) {
		dpu_err("slide windows 0x%llxx\n", regs_args->reg_addr);
		iatu_map_inbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1,
				     regs_args->reg_addr, CONFIG_BAR_SIZE);
		iatu_map_outbound_bar(dpu_dev, CONFIG_BAR_INDEX, 1,
				      regs_args->reg_addr, CONFIG_BAR_SIZE);
		dpu_dev->config_bar_base = regs_args->reg_addr;
	}
#endif
	dpu_dbg("regs addr+++ %llx data %x\n", regs_args->reg_addr,
		regs_args->reg_data);
	switch (regs_args->rw_flag) {
	case REG_WRITE:
		eon_writel(regs_args->reg_data,
			   offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	case REG_READ:
		regs_args->reg_data = eon_readl(
			offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	case REG_WRITE16:
		eon_write16(regs_args->reg_data,
			   offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	case REG_READ16:
		regs_args->reg_data = eon_read16(
			offset + dpu_dev->bar[CONFIG_BAR_INDEX].vaddr);
		smp_mb();
		break;
	default:
		dpu_err("Unknow regs operation\n");
		break;
	}
	dpu_dbg("regs addr--- %llx data %x\n", regs_args->reg_addr,
		regs_args->reg_data);

	if (!ret && (cmd & IOC_OUT)) {
		if (copy_to_user((void __user *)args, regs_args,
				 sizeof(struct dpu_reg_args))) {
			dpu_err("failed to copy to user\n");
			ret = -3;
		}
	}

	mutex_unlock(&dpu_dev->conbar_lock);
	kfree(regs_args);

	return ret;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct eon_device *dpu_dev = file->private_data;
	int ret = 0;

	dpu_dbg("%s %d\n", __func__, __LINE__);
	switch (cmd) {
	case EON_IOCTL_HDMA:
		ret = eon_hdma(dpu_dev, cmd, arg);
		break;
	case EON_IOCTL_IATU:
		ret = eon_rw_reg(dpu_dev, cmd, arg);
		break;
	default:
		pr_err("unknow ioctl cmd 0x%x 0x%x\n", EON_IOCTL_TEST, cmd);
		ret = -ENOTTY;
		break;
	} /* switch */

	//while(atomic_read(&test_ctx.flag));
	dpu_dbg("%s %d ioctl test end!!!\n", __func__, __LINE__);

	return ret;
}

static const struct file_operations device_ops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.llseek = default_llseek,
};

int char_dev_init(struct eon_device *eon)
{
	struct char_dev_ctx *ctx;
	int ret = 0;
	char *name;
	// int i = 0;

	ctx = kzalloc(sizeof(struct char_dev_ctx), GFP_KERNEL);
	if (unlikely(!ctx)) {
		return -EPERM;
	}
	//ctx->magic = 0x55;
	//ctx->major = MAJOR(dev);

	cdev_init(&ctx->dev, &device_ops);
	ctx->dev.owner = THIS_MODULE;
	ret = cdev_add(&ctx->dev, MKDEV(eon_dpu_major, eon->dpu_id), 1);
	if (ret < 0) {
		pr_err("failed to add zeus test\n");
		goto err_cdev_add;
	}

	name = kasprintf(GFP_KERNEL, "dpu%d", eon->dpu_id);
	ctx->char_dev = device_create(eon_class, NULL,
				      MKDEV(eon_dpu_major, eon->dpu_id), ctx,
				      "%s", name);
	if (!ctx->char_dev) {
		pr_err("failed to create device %s\n", name);
		kfree(name);
		goto err_device_create;
		return -EPERM;
	}
	kfree(name);

	ctx->dpu_dev = eon;
	eon->char_ctx = ctx;

	return 0;
err_device_create:
	cdev_del(&ctx->dev);
err_cdev_add:
	return ret;
}

int char_dev_fini(struct eon_device *eon)
{
	struct char_dev_ctx *ctx = eon->char_ctx;

	device_destroy(eon_class, MKDEV(eon_dpu_major, eon->dpu_id));
	cdev_del(&ctx->dev);

	return 0;
}
