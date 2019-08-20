#include "et_dma.h"

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include "hal_device.h"

//TODO: define all of these structures / constants shared between the Linux driver
//and SoC firmware somewhere common. Note the Linux driver needs to be self-contained
//(no dependencies) if we are going to mainline it.

struct dma_run_to_done_message_t {
	uint64_t message_id;
	uint32_t chan;
};

struct dma_done_message_t {
	uint64_t message_id;
	uint32_t chan;
	int32_t status;
};

#define DMA_LL_SIZE 0x800000

#define DMA_CHAN_READ_0_LL_BASE 0x87FC000000
#define DMA_CHAN_READ_1_LL_BASE 0x87FC800000
#define DMA_CHAN_READ_2_LL_BASE 0x87FD000000
#define DMA_CHAN_READ_3_LL_BASE 0x87FD800000
#define DMA_CHAN_WRITE_0_LL_BASE 0x87FE000000
#define DMA_CHAN_WRITE_1_LL_BASE 0x87FE800000
#define DMA_CHAN_WRITE_2_LL_BASE 0x87FF000000
#define DMA_CHAN_WRITE_3_LL_BASE 0x87FF800000

static uint64_t XFER_LIST_OFFSETS[] = {
	DMA_CHAN_READ_0_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_READ_1_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_READ_2_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_READ_3_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_WRITE_0_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_WRITE_1_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_WRITE_2_LL_BASE - R_L3_DRAM_BASEADDR,
	DMA_CHAN_WRITE_3_LL_BASE - R_L3_DRAM_BASEADDR
};

#define MBOX_MESSAGE_ID_DMA_RUN_TO_DONE 9
#define MBOX_MESSAGE_ID_DMA_DONE 10
//End shared elements between SoC and Linux driver

struct et_mem_region {
	uint64_t begin;
	uint64_t end;
};

static const struct et_mem_region valid_dma_targets[] = {
	//L3, but beginning reserved for minion stacks, end reserved for DMA config
	{ .begin = 0x8200000000, .end = 0x87FBFFFFFF },
	//Shire-cache scratch pads. Limit to first 4MB * 33 shires.
	{ .begin = 0x80000000, .end = 0x88400000}
};

int et_dma_bounds_check(uint64_t soc_addr, uint64_t count)
{
	int i;
	uint64_t end_addr = soc_addr + count - 1;

	//Check for u64 overflow
	if (end_addr < soc_addr)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(valid_dma_targets); ++i) {
		if (soc_addr >= valid_dma_targets[i].begin &&
		    end_addr <= valid_dma_targets[i].end) {
			return 0;
		}
	}
	//Made it through list without finding match
	pr_err("DMA target 0x%010llx - 0x%010llx invalid\n", soc_addr,
	       end_addr);
	return -EINVAL;
}

//static: dma_pull_contig_buff(dma_buffer_t, soc_addr, count) - might break user's write into multiple DMAs as not to pin their whole buffer. MMIO's 1 element sgl as well.
//static: pull_contig_config_xfer_list
//static: pull_wait_for_done (eventually, uses wait_queue triggered by IRQ)

#define CTRL_CB 0x01
#define CTRL_TCB 0x02
#define CTRL_LLP 0x04
#define CTRL_LIE 0x08
#define CTRL_RIE 0x10

#define DATA_CTRL 0
#define DATA_SIZE 4
#define DATA_SAR_LOW 8
#define DATA_SAR_HIGH 12
#define DATA_DAR_LOW 16
#define DATA_DAR_HIGH 20

#define LINK_CTRL 0
#define LINK_PTR_LOW 8
#define LINK_PTR_HIGH 12

#define XFER_NODE_BYTES 24

/*
 * Writes one data element of a DMA tranfser list via MMIO. This structure is used
 * to configure the DMA engine.
 */
static void write_xfer_list_data(enum et_dma_chan chan, uint32_t i,
				 uint64_t sar, uint64_t dar, uint32_t size,
				 bool lie, void __iomem *r_drct_dram)
{
	uint64_t offset = XFER_LIST_OFFSETS[chan] + i * XFER_NODE_BYTES;

	uint32_t ctrl_dword = CTRL_CB | CTRL_LIE;

	iowrite32(ctrl_dword, r_drct_dram + offset + DATA_CTRL);
	iowrite32(size, r_drct_dram + offset + DATA_SIZE);
	iowrite32((u32)sar, r_drct_dram + offset + DATA_SAR_LOW);
	iowrite32((u32)(sar >> 32), r_drct_dram + offset + DATA_SAR_HIGH);
	iowrite32((u32)dar, r_drct_dram + offset + DATA_DAR_LOW);
	iowrite32((u32)(dar >> 32), r_drct_dram + offset + DATA_DAR_HIGH);
}

/*
 * Writes one data element of a DMA tranfser list via MMIO. This structure is used
 * to configure the DMA engine.
 */
static void write_xfer_list_link(enum et_dma_chan chan, uint32_t i,
				 uint64_t ptr, void __iomem *r_drct_dram)
{
	uint64_t offset = XFER_LIST_OFFSETS[chan] + i * XFER_NODE_BYTES;

	uint32_t ctrl_dword = CTRL_TCB | CTRL_LLP;

	iowrite32(ctrl_dword, r_drct_dram + offset + LINK_CTRL);
	iowrite32((u32)ptr, r_drct_dram + offset + LINK_PTR_LOW);
	iowrite32((u32)(ptr >> 32), r_drct_dram + offset + LINK_PTR_HIGH);
}

static ssize_t et_dma_contig_buff(dma_addr_t buff, uint64_t soc_addr,
				  ssize_t count, enum et_dma_chan chan,
				  struct et_mbox *mbox_mm,
				  void __iomem *r_drct_dram)
{
	ssize_t rv;
	struct dma_run_to_done_message_t run_msg = {
		.message_id = MBOX_MESSAGE_ID_DMA_RUN_TO_DONE,
		.chan = (uint32_t)chan
	};
	struct dma_done_message_t done_msg;
	uint64_t src_addr, dest_addr;

	/* read: source is on host, dest is on SoC */
	if (chan <= et_dma_chan_read_3) {
		src_addr = (uint64_t)buff;
		dest_addr = soc_addr;
	}
	/* write: source is one SoC, dest is on host */
	else {
		src_addr = soc_addr;
		dest_addr = (uint64_t)buff;
	}

	//Use MMIO to write a single-data-element xfer list
	write_xfer_list_data(chan, 0, src_addr, dest_addr, count,
			     true /*interrupt*/, r_drct_dram);
	write_xfer_list_link(chan, 1,
			     R_L3_DRAM_BASEADDR +
				     XFER_LIST_OFFSETS[chan] /*circular link*/,
			     r_drct_dram);

	//This code relies on mbox write doing a pcie read to make sure all of the
	//linked list node writes are done before DMA is started.

	//Signal the MM to start and supervise the DMA transefer
	rv = et_mbox_write(mbox_mm, &run_msg, sizeof(run_msg));
	if (rv < 0) {
		pr_err("mbox write errored\n");
		return rv;
	}
	if (rv != sizeof(run_msg)) {
		pr_err("mbox write didn't send all bytes\n");
		return -EIO;
	}

	//Wait for the DMA transfer to be done
	//TODO: for now, poll on the next message coming in (assumes this is
	//the only one using mbox, which will be true because rw_mutex.
	//Eventually, this should use a wait queue and incoming messages should
	//be dispatched from the mbox ISR by message_id).
	do {
		msleep(100);
		rv = et_mbox_read(mbox_mm, &done_msg, sizeof(done_msg));
	} while (rv == 0);

	if (rv < 0) {
		pr_err("mbox read errored\n");
		return rv;
	}
	if (rv != sizeof(done_msg)) {
		pr_err("mbox read didn't receive alll bytes\n");
		return -EIO;
	}

	if (done_msg.message_id != MBOX_MESSAGE_ID_DMA_DONE) {
		pr_err("got wrong message (%llu)\n", done_msg.message_id);
		return -EIO;
	}
	if (done_msg.chan != chan) {
		pr_err("got DMA done message for wrong chan (%d, expected %d)\n",
		       done_msg.chan, chan);
		return -EIO;
	}
	if (done_msg.status != 0) {
		pr_err("DMA transfer failed\n");
		return done_msg.status;
	}

	return count;
}

ssize_t et_dma_pull_from_user(const char __user *buf, size_t count, loff_t *pos,
			      enum et_dma_chan chan, struct et_mbox *mbox_mm,
			      void __iomem *r_drct_dram, struct pci_dev *pdev)
{
	ssize_t rv;
	uint64_t soc_addr = (uint64_t)*pos;
	uint8_t *kern_buff;
	dma_addr_t kern_buff_addr;

	//TODO: resource reservation. If DMA channel is in use, don't
	//reconfigure mid operation! For now, only one DMA can take place at
	//once, and each DMA blocks until completion, so there is no issue.

	if (chan < et_dma_chan_read_0 || chan > et_dma_chan_read_3) {
		pr_err("invalid DMA read channel %d\n", chan);
		return -EINVAL;
	}

	//Bounds check transfer
	rv = et_dma_bounds_check(soc_addr, (uint64_t)count);
	if (rv != 0) {
		pr_err("bounds check failed\n");
		return rv;
	}

	if (count > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	//Allocate a contigious buffer to hold user's data
	//TODO: short-term: use the non-coherant API for perfomance
	//TODO: long-term: the buffer from user mode might be quite large (GB).
	//Copying the whole thing might be very bad, both for exec time and
	//memory pressure. Instead, pin/DMA it in little chunks at a time.
	kern_buff = dma_alloc_coherent(&pdev->dev, count, &kern_buff_addr, GFP_KERNEL);

	if (!kern_buff) {
		pr_err("dma alloc failed\n");
		return -ENOMEM;
	}

	//Copy the buffer from user-mode
	rv = copy_from_user(kern_buff, buf, count);
	if (rv != 0 ) {
		pr_err("Failed to copy from user\n");
		goto error;
	}

	rv = et_dma_contig_buff(kern_buff_addr, soc_addr, count, chan, mbox_mm,
				r_drct_dram);

error:
	//Free the buffer
	dma_free_coherent(&pdev->dev, count, kern_buff, kern_buff_addr);

	if (rv > 0) *pos += rv;

	return rv;
}

ssize_t et_dma_push_to_user(char __user *buf, size_t count, loff_t *pos,
			    enum et_dma_chan chan, struct et_mbox *mbox_mm,
			    void __iomem *r_drct_dram, struct pci_dev *pdev)
{
	ssize_t rv, dma_cnt;
	uint64_t soc_addr = (uint64_t)*pos;
	uint8_t *kern_buff;
	dma_addr_t kern_buff_addr;

	//TODO: resource reservation. If DMA channel is in use, don't
	//reconfigure mid operation! For now, only one DMA can take place at
	//once, and each DMA blocks until completion, so there is no issue.

	if (chan < et_dma_chan_write_0 || chan > et_dma_chan_write_3) {
		pr_err("invalid DMA read channel %d\n", chan);
		return -EINVAL;
	}

	//Bounds check transfer
	rv = et_dma_bounds_check(soc_addr, (uint64_t)count);
	if (rv != 0) {
		pr_err("bounds check failed\n");
		return rv;
	}

	if (count > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	//Allocate a contigious buffer to hold user's data
	//TODO: short-term: use the non-coherant API for perfomance
	//TODO: long-term: the buffer from user mode might be quite large (GB).
	//Copying the whole thing might be very bad, both for exec time and
	//memory pressure. Instead, pin/DMA it in little chunks at a time.
	kern_buff = dma_alloc_coherent(&pdev->dev, count, &kern_buff_addr, GFP_KERNEL);

	if (!kern_buff) {
		pr_err("dma alloc failed\n");
		return -ENOMEM;
	}

	dma_cnt = et_dma_contig_buff(kern_buff_addr, soc_addr, count, chan,
				     mbox_mm, r_drct_dram);
	if (dma_cnt < 0 ) {
		goto error;
	}

	//Copy the buffer to user-mode
	rv = copy_to_user(buf, kern_buff, count);
	if (rv != 0) {
		pr_err("Failed to copy to user\n");
		dma_cnt = rv;
	}

error:
	//Free the buffer
	dma_free_coherent(&pdev->dev, count, kern_buff, kern_buff_addr);

	if (dma_cnt > 0) *pos += dma_cnt;

	return dma_cnt;
}