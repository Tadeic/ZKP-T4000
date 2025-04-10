// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (c) 2001,2002 Christer Weinigel <wingel@nano-system.com>
 *
 *  National Semiconductor eon support.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/pci-epf.h>
#include <linux/kthread.h>
#include <linux/msi.h>
#include "eon_drv.h"
#include "test_dev.h"
#include "iatu.h"
#include "hdma.h"

#ifndef pr_fmt
#define pr_fmt(fmt) "eon-drv: " fmt
#endif

struct class *eon_class;
unsigned int eon_dpu_major;

uint32_t eon_readl(void *addr)
{
	uint32_t data = ioread32(addr);
	//dpu_dbg("----read addr 0x%llx data 0x%x\n", (uint64_t)addr, data);
	return data;
}

void eon_writel(unsigned int data, void *addr)
{
	iowrite32(data, addr);

	//dpu_dbg("++++write addr 0x%llx data 0x%x\n", (uint64_t)addr, data);
}
uint16_t eon_read16(void *addr)
{
	uint16_t data = ioread16(addr);
	//dpu_dbg("----read addr 0x%llx data 0x%x\n", (uint64_t)addr, data);
	return data;
}

void eon_write16(uint16_t data, void *addr)
{
	iowrite16(data, addr);

	//dpu_dbg("++++write addr 0x%llx data 0x%x\n", (uint64_t)addr, data);
}
void hdma_writel(void *hdma_base, unsigned int data, void *addr)
{
	iowrite32(data, addr);

	if (addr >= hdma_base && addr < hdma_base + 0x8000)
		dpu_dbg("write addr 0x%llx data 0x%x\n",
			(uint64_t)(addr - hdma_base), data);
}

static int set_dma_mask(struct pci_dev *pdev)
{
	int err;

	err = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (!err) {
		dpu_info("Using a 64-bit DMA mask.\n");
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err) {
			pci_err(pdev, "consistent DMA mask 64 set failed\n");
			return err;
		}
	} else {
		dpu_info("Could not set 64-bit DMA mask.\n");
		err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (!err) {
			err = pci_set_consistent_dma_mask(pdev,
							  DMA_BIT_MASK(32));
			if (err) {
				pci_err(pdev,
					"consistent DMA mask 32 set failed\n");
				return err;
			}
		}
	}

	return 0;
}

static void pci_msi_setup(struct eon_device *eon)
{
	uint32_t pci_msi_addr_lo, pci_msi_addr_hi = 0;
	struct pci_dev *pdev = eon->pdev;
	int offset;
	uint16_t msg_ctl, data;

	pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_FLAGS, &msg_ctl);
	offset = PCI_MSI_DATA_32;
	if (msg_ctl & PCI_MSI_FLAGS_64BIT)
		offset = PCI_MSI_DATA_64;
	pci_read_config_word(pdev, pdev->msi_cap + offset, &data);
	eon->base_addr.msg_data = data;

	pci_read_config_dword(pdev, pdev->msi_cap + PCI_MSI_ADDRESS_LO,
			      &pci_msi_addr_lo);
	if (msg_ctl & PCI_MSI_FLAGS_64BIT) {
		pci_read_config_dword(pdev, pdev->msi_cap + PCI_MSI_ADDRESS_HI,
				      &pci_msi_addr_hi);
	}

	eon->base_addr.msi_addr_l = pci_msi_addr_lo;
	eon->base_addr.msi_addr_h = pci_msi_addr_hi;
	//dpu_dbg("offset %x msr addr high %x, low %x, data %x\n", offset, pci_msi_addr_hi, pci_msi_addr_lo, data);
}

void iatu_config(struct eon_device *eon)
{
	iatu_map_inbound_bar(eon, DESC_BAR, 0, IRAM_BASE_SOC_ADDR, 512 * 1024);
	iatu_map_outbound_bar(eon, DESC_BAR, 0, IRAM_BASE_SOC_ADDR, 512 * 1024);
}

//#define DISABLE_THREAD_HANDLE
irqreturn_t dma_isr_handler(int irq, void *data)
{
	struct eon_device *eon = (struct eon_device *)data;
	//uint32_t irq_status, i, j;
	uint32_t retry = 3, flag = 0;
	uint8_t data_tmp = 0x55;

	dpu_dbg("hdma irq handle ++\n\n");
#ifdef DISABLE_THREAD_HANDLE
	for (j = 0; j < retry; j++) {
		for (i = 0; i < MAX_HDMA_NUMBER; i++) {
			irq_status = hdma_get_intr_status(eon->hdmar[i]);
			if ((irq_status & 7) != 0) {
				flag |= irq_status;
				dpu_err("wake up %d rd wq\n", i);
				eon->hdmar[i]->state = TRANSFER_STATE_COMPLETED;
				hdma_clear_intr_status(eon->hdmar[i],
						       irq_status);
				if ((flag & 1) == 1) {
					wake_up_interruptible(&eon->hdmar[i]->wq);
				}
				return IRQ_HANDLED;
			}

			irq_status = hdma_get_intr_status(eon->hdmaw[i]);
			if ((irq_status & 7) != 0) {
				flag |= irq_status;
				dpu_err("wake up %d wr wq\n", i);
				eon->hdmaw[i]->state = TRANSFER_STATE_COMPLETED;
				hdma_clear_intr_status(eon->hdmaw[i],
						       irq_status);
				if ((flag & 1) == 1) {
					wake_up_interruptible(&eon->hdmaw[i]->wq);
				}
				return IRQ_HANDLED;
			}
		}

		if (flag)
			break;
	}
#else
	kfifo_in(&eon->hdma_kfifo, &data_tmp, sizeof(data_tmp));
	wake_up_interruptible(&eon->thread_wq);
#endif

	dpu_dbg("hdma irq handle --\n\n");

	return IRQ_HANDLED;
}

int eon_irq_init(struct eon_device *eon)
{
	struct pci_dev *pdev = eon->pdev;
	int ret = 0;

	eon->irqs = pci_alloc_irq_vectors(eon->pdev, 1, EON_MSI_IRQ_NUM,
					  PCI_IRQ_MSI);
	if (eon->irqs < 1) {
		pci_err(eon->pdev,
			"fail to alloc IRQ vector (number of IRQs=%u)\n",
			eon->irqs);
		return -EPERM;
	}

	ret = devm_request_irq(&eon->pdev->dev, pci_irq_vector(pdev, 0),
			       dma_isr_handler, IRQF_SHARED, "dpu-dma", eon);
	if (ret) {
		pr_err("failed to requster irq\n");
		goto err_request_irq;
	}

	pci_msi_setup(eon);


	return 0;
err_request_irq:
	pci_free_irq_vectors(eon->pdev);
	return ret;
}

void eon_irq_fini(struct eon_device *eon)
{
	struct pci_dev *pdev = eon->pdev;

	devm_free_irq(&pdev->dev, pci_irq_vector(pdev, 0), eon);
	pci_free_irq_vectors(eon->pdev);
}

static int hama_handle_kthread(void *arg)
{
	struct eon_device *eno_dev = (struct eon_device *)arg;
	int ret = 0, loop = 0;
	uint32_t irq_status, i;
	uint8_t flag = 0;
	uint8_t data = 0;

	while (1) {
		ret = wait_event_interruptible_timeout(
			eno_dev->thread_wq,
			kfifo_len(&eno_dev->hdma_kfifo) >= sizeof(uint8_t) ||
				kthread_should_stop(),
			msecs_to_jiffies(200));

		if (kthread_should_stop()) {
			break;
		}

		while (kfifo_len(&eno_dev->hdma_kfifo) >= sizeof(uint8_t)) {
			loop++;
			for (i = 0; i < MAX_HDMA_NUMBER; i++) {
				irq_status =
					hdma_get_intr_status(eno_dev->hdmar[i]);
				if ((irq_status & 7) != 0) {
					flag = irq_status & 3;
					dpu_dbg("wake up %d read flag %x\n", i,
						flag);
					eno_dev->hdmar[i]->state =
						TRANSFER_STATE_COMPLETED;
					hdma_clear_intr_status(
						eno_dev->hdmar[i], irq_status);
					if ((flag & 1) == 1) {
						wake_up_interruptible(
							&eno_dev->hdmar[i]->wq);
					}
					loop = 0;
					break;
				}

				irq_status =
					hdma_get_intr_status(eno_dev->hdmaw[i]);
				if ((irq_status & 7) != 0) {
					flag = irq_status & 3;
					dpu_dbg("wake up %d write flag %x\n", i,
						flag);
					eno_dev->hdmaw[i]->state =
						TRANSFER_STATE_COMPLETED;
					hdma_clear_intr_status(
						eno_dev->hdmaw[i], irq_status);
					if ((flag & 1) == 1) {
						wake_up_interruptible(
							&eno_dev->hdmaw[i]->wq);
					}
					loop = 0;
					break;
				}
			}

			if (flag == 3) {
				dpu_dbg("eno kfifo out\n");
				ret = kfifo_out(&eno_dev->hdma_kfifo, &data,
						sizeof(uint8_t));
				if (ret != sizeof(uint8_t)) {
					dpu_err("error %s %d\n", __func__,
						__LINE__);
				}
				ret = kfifo_out(&eno_dev->hdma_kfifo, &data,
						sizeof(uint8_t));
				if (ret != sizeof(uint8_t)) {
					dpu_dbg("error %s %d\n", __func__,
						__LINE__);
				}
				flag = 0;
			}
			if (flag == 2 || flag == 1) {
				dpu_dbg("eno kfifo out\n");
				ret = kfifo_out(&eno_dev->hdma_kfifo, &data,
						sizeof(uint8_t));
				if (ret != sizeof(uint8_t)) {
					dpu_err("error %s %d\n", __func__,
						__LINE__);
				}
				flag = 0;
			}

			if (loop >= 32) {
				ret = kfifo_out(&eno_dev->hdma_kfifo, &data,
						sizeof(uint8_t));
				if (ret != sizeof(uint8_t)) {
					dpu_err("error %s %d\n", __func__,
						__LINE__);
				}
				loop = 0;
				break;
			}

			if (eno_dev->should_stop_flag == 1) {
				break;
			}
		}

#ifndef DISABLE_THREAD_HANDLE
		//Add extra loop
		if (kfifo_len(&eno_dev->hdma_kfifo) < sizeof(uint8_t)) {
			for (i = 0; i < MAX_HDMA_NUMBER; i++) {
				irq_status =
					hdma_get_intr_status(eno_dev->hdmar[i]);
				if ((irq_status & 7) != 0) {
					dpu_dbg("wake up %d read flag %x\n", i,
						flag);
					eno_dev->hdmar[i]->state =
						TRANSFER_STATE_COMPLETED;
					hdma_clear_intr_status(
						eno_dev->hdmar[i], irq_status);
					wake_up_interruptible(
						&eno_dev->hdmar[i]->wq);
					break;
				}

				irq_status =
					hdma_get_intr_status(eno_dev->hdmaw[i]);
				if ((irq_status & 7) != 0) {
					dpu_dbg("wake up %d write flag %x\n", i,
						flag);
					eno_dev->hdmaw[i]->state =
						TRANSFER_STATE_COMPLETED;
					hdma_clear_intr_status(
						eno_dev->hdmaw[i], irq_status);
					wake_up_interruptible(
						&eno_dev->hdmaw[i]->wq);
					break;
				}
			}
		}
#endif
		if (eno_dev->should_stop_flag == 1) {
			break;
		}
	}

	return ret;
}

static uint32_t get_dpu_id(void)
{
	static uint32_t dpu_id = 0;
	uint32_t ret_dpu_id = dpu_id++;
	return ret_dpu_id;
}
 
static int eon_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct device *dev = &pdev->dev;
	struct eon_device *eon_dev;
	void __iomem *const *iomap;
	int ret = 0;
	int i = 0;

	dpu_dbg("%s %d++\n", __func__, __LINE__);

	ret = pcim_enable_device(pdev);
	if (ret) {
		pci_err(pdev, "pcim_enable_device() failed, %d.\n", ret);
		return ret;
	}

	pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_RELAX_EN);

	ret = pcie_set_readrq(pdev, 4096);
	if (ret)
		pr_info("device %s, error set PCI_EXP_DEVCTL_READRQ: %d.\n",
			dev_name(&pdev->dev), ret);

	ret = pcim_iomap_regions(pdev, BIT(BAR_0) | BIT(BAR_2) | BIT(BAR_4),
				 pci_name(pdev));
	if (ret) {
		pci_err(pdev, "eDMA BAR I/O remapping failed\n");
		return ret;
	}

	pci_set_master(pdev);

	ret = set_dma_mask(pdev);
	if (ret) {
		pci_err(pdev, "set eon dma mask failed %d\n", ret);
		goto exit;
	}

	eon_dev = devm_kzalloc(dev, sizeof(*eon_dev), GFP_KERNEL);
	if (!eon_dev)
		return -EPERM;
	eon_dev->pdev = pdev;
	eon_dev->dpu_id = get_dpu_id();
	pci_set_drvdata(pdev, eon_dev);

	iomap = pcim_iomap_table(pdev);
	for (i = 0; i <= BAR_5; i++) {
		eon_dev->bar[i].paddr = pci_resource_start(pdev, i);
		eon_dev->bar[i].vaddr = iomap[i];
		eon_dev->bar[i].sz = pci_resource_len(pdev, i);
		pci_err(pdev,
			"Registers:\tBAR=%u, sz=0x%zx bytes, addr(v=%p, p=%llx)\n",
			i, eon_dev->bar[i].sz, eon_dev->bar[i].vaddr,
			eon_dev->bar[i].paddr);
	}

	eon_dev->base_addr.iatu_base =
		eon_dev->bar[ATU_HDMA_BAR].vaddr + IATU_BASE_OFFSET;
	eon_dev->base_addr.hdma_base =
		eon_dev->bar[ATU_HDMA_BAR].vaddr + HDMA_BASE_BAR_OFFSET;

	init_waitqueue_head(&eon_dev->thread_wq);
	ret = eon_irq_init(eon_dev);
	if (ret) {
		pci_err(pdev, "%s %d--", __func__, __LINE__);
		goto err_irq_init;
	}

	mutex_init(&eon_dev->conbar_lock);
	iatu_config(eon_dev);

	ret = char_dev_init(eon_dev);
	if (ret) {
		pci_err(pdev, "char_dev_init failed %d\n", ret);
		goto err_char_dev;
	}

	if (kfifo_alloc(&eon_dev->hdma_kfifo, 4096, GFP_KERNEL)) {
		pci_err(pdev, "%s %d--", __func__, __LINE__);
		goto err_kfifo;
	}

	eon_dev->should_stop_flag = 0;
	eon_dev->hdma_thread =
		kthread_run(hama_handle_kthread, eon_dev, "hdma_down");
	if (eon_dev->hdma_thread == ERR_PTR(-ENOMEM)) {
		ret = -ENOMEM;
		pci_err(pdev, "kthread run error %d\n", ret);
		goto err_kthread_run;
	}

	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		ret = hdma_init(&eon_dev->hdmar[i], eon_dev, i,
				eon_dev->base_addr.hdma_base, 0);
		if (ret) {
			pci_err(pdev, "hdma_init %d failed %d\n", i, ret);
			goto exit;
		}
		hdma_enable(eon_dev->hdmar[i]);
		hdma_msi_interrupt(eon_dev->hdmar[i],
				   eon_dev->base_addr.msi_addr_h,
				   eon_dev->base_addr.msi_addr_l,
				   eon_dev->base_addr.msg_data);
	}

	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		ret = hdma_init(&eon_dev->hdmaw[i], eon_dev, i,
				eon_dev->base_addr.hdma_base, 1);
		if (ret) {
			pci_err(pdev, "hdma_init %d failed %d\n", i, ret);
			goto exit;
		}

		hdma_enable(eon_dev->hdmaw[i]);
		hdma_msi_interrupt(eon_dev->hdmaw[i],
				   eon_dev->base_addr.msi_addr_h,
				   eon_dev->base_addr.msi_addr_l,
				   eon_dev->base_addr.msg_data);
	}

	return 0;

exit:
	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		if (eon_dev->hdmaw[i]) {
			hdma_exit(eon_dev->hdmaw[i]);
			eon_dev->hdmaw[i] = NULL;
		}
	}
	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		if (eon_dev->hdmar[i]) {
			hdma_exit(eon_dev->hdmar[i]);
			eon_dev->hdmar[i] = NULL;
		}
	}
	kthread_stop(eon_dev->hdma_thread);
err_kthread_run:
	kfifo_free(&eon_dev->hdma_kfifo);
err_kfifo:
	char_dev_fini(eon_dev);
err_char_dev:
	pci_free_irq_vectors(pdev);
err_irq_init:

	return ret;
}

static void eon_remove(struct pci_dev *pdev)
{
	int i;
	struct eon_device *dev = pci_get_drvdata(pdev);

	dpu_dbg("%s %d.n", __func__, __LINE__);

	dev->should_stop_flag = 1;
	kthread_stop(dev->hdma_thread);
	wake_up_interruptible(&dev->thread_wq);
	eon_irq_fini(dev);
	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		if (dev->hdmaw[i]) {
			hdma_exit(dev->hdmaw[i]);
			dev->hdmaw[i] = NULL;
		}
	}

	for (i = 0; i < MAX_HDMA_NUMBER; i++) {
		if (dev->hdmar[i]) {
			hdma_exit(dev->hdmar[i]);
			dev->hdmar[i] = NULL;
		}
	}
	kfifo_free(&dev->hdma_kfifo);
	char_dev_fini(dev);
	pci_free_irq_vectors(dev->pdev);
}

static struct pci_device_id eon_tbl[] = { {
						  PCI_DEVICE(EON_VENDOR_ID,
							     EON_DEVICE_ID),
					  },
					  {
						  0,
					  } };
MODULE_DEVICE_TABLE(pci, eon_tbl);

static struct pci_driver eon_pci_driver = {
	.name = "eon",
	.id_table = eon_tbl,
	.probe = eon_probe,
	.remove = eon_remove,
};

static int __init eon_init(void)
{
	int ret = 0;
	dev_t dev;

	dpu_info("init eon Driver\n");
	ret = alloc_chrdev_region(&dev, 0, EON_TEST_MINORS, EON_TEST);
	if (ret < 0) {
		pr_err("alloc zeus test major failed\n");
		return ret;
	}

	eon_dpu_major = MAJOR(dev);
	eon_class = class_create(THIS_MODULE, EON_TEST);
	if (IS_ERR(eon_class)) {
		pr_err("failed to create class\n");
		ret = PTR_ERR(eon_class);
		goto err_class_creat;
	}

	ret = pci_register_driver(&eon_pci_driver);
	if (ret) {
		goto err_pci_register;
	}

	return 0;

err_pci_register:
	class_destroy(eon_class);
err_class_creat:
	unregister_chrdev_region(MKDEV(eon_dpu_major, 0), EON_TEST_MINORS);
	return ret;
}

static void __exit eon_exit(void)
{
	pci_unregister_driver(&eon_pci_driver);
	class_destroy(eon_class);
	unregister_chrdev_region(MKDEV(eon_dpu_major, 0), EON_TEST_MINORS);
}

module_init(eon_init);
module_exit(eon_exit);

MODULE_AUTHOR("Christer Weinigel <wingel@eon.com>");
MODULE_DESCRIPTION("eon Driver");
MODULE_LICENSE("GPL");
