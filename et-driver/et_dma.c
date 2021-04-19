#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include "et_io.h"
#include "et_dma.h"
#include "et_vqueue.h"
#include "et_pci_dev.h"

// TODO: Enable/Remove while implementing DMA Scatter/Gather
// We might need old DMA transfer list based approach for DMA Scatter/Gather
#if 0
//TODO: define all of these structures / constants shared between the Linux driver
//and SoC firmware somewhere common. Note the Linux driver needs to be self-contained
//(no dependencies) if we are going to mainline it.

static uint64_t XFER_LIST_OFFSETS[] = {
	DMA_CHAN_READ_0_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_READ_1_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_READ_2_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_READ_3_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_WRITE_0_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_WRITE_1_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_WRITE_2_LL_BASE - DRAM_MEMMAP_BEGIN,
	DMA_CHAN_WRITE_3_LL_BASE - DRAM_MEMMAP_BEGIN
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
	{ .begin = HOST_MANAGED_DRAM_START, .end = (HOST_MANAGED_DRAM_END - 1) },
	//Shire-cache scratch pads. Limit to first 4MB * 33 shires.
	{ .begin = 0x80000000, .end = 0x883FFFFF }
};

void et_dma_init(struct et_pci_dev *et_dev)
{
	int i;

	for (i = 0; i < ET_DMA_NUM_CHANS; ++i) {
		mutex_init(&et_dev->dma_chans[i].state_mutex);
		init_waitqueue_head(&et_dev->dma_chans[i].wait_queue);
	}

	//TODO: JIRA SW-1153: Cleanup the DMA hardware if it was previously aborted mid-DMA.
}

void et_dma_destroy(struct et_pci_dev *et_dev)
{
	int i;

	for (i = 0; i < ET_DMA_NUM_CHANS; ++i) {
		bool wait_for_cleanup = false;

		mutex_lock(&et_dev->dma_chans[i].state_mutex);

		if (et_dev->dma_chans[i].state == ET_DMA_STATE_ACTIVE) {
			wait_for_cleanup = true;
			et_dev->dma_chans[i].state = ET_DMA_STATE_ABORTING;
		} else {
			et_dev->dma_chans[i].state = ET_DMA_STATE_ABORTED;
		}
		
		mutex_unlock(&et_dev->dma_chans[i].state_mutex);

		wake_up_interruptible_all(&et_dev->dma_chans[i].wait_queue);

		if (wait_for_cleanup) {
			while (et_dma_get_state(i, et_dev) != ET_DMA_STATE_ABORTED) {
				usleep_range(1, 10);
			}
		}

		mutex_destroy(&et_dev->dma_chans[i].state_mutex);
	}
}

enum ET_DMA_STATE et_dma_get_state(enum ET_DMA_CHAN_ID id, struct et_pci_dev *et_dev)
{
	enum ET_DMA_STATE rv;

	if (id < ET_DMA_CHAN_ID_READ_0 || id > ET_DMA_CHAN_ID_WRITE_3) {
		return ET_DMA_STATE_ABORTED;
	}

	mutex_lock(&et_dev->dma_chans[id].state_mutex);
	rv = et_dev->dma_chans[id].state;
	mutex_unlock(&et_dev->dma_chans[id].state_mutex);

	return rv;
}

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

//Control Register Bitfields:

//Cycle Bit
#define CTRL_CB 0x01
//Toggle Cycle Bit
#define CTRL_TCB 0x02
//Load Link Pointer
#define CTRL_LLP 0x04
//Local Interrupt Enable
#define CTRL_LIE 0x08
//Remote Interrupt Enable
#define CTRL_RIE 0x10

#define DATA_CTRL 0
#define DATA_SIZE 4
#define DATA_SAR 8
#define DATA_DAR 16

#define LINK_CTRL 0
#define LINK_PTR 8

#define XFER_NODE_BYTES 24

/*
 * Writes one data element of a DMA tranfser list via MMIO. This structure is used
 * to configure the DMA engine.
 */
static void write_xfer_list_data(enum ET_DMA_CHAN_ID id, uint32_t i,
				 uint64_t sar, uint64_t dar, uint32_t size,
				 bool lie, struct et_pci_dev *et_dev)
{
	void __iomem *offset = et_dev->iomem[IOMEM_R_DRCT_DRAM] +
			       XFER_LIST_OFFSETS[id] + i * XFER_NODE_BYTES;

	uint32_t ctrl_dword = CTRL_CB;
	if (lie) ctrl_dword |= CTRL_LIE;

	iowrite32(ctrl_dword, offset + DATA_CTRL);
	iowrite32(size, offset + DATA_SIZE);
	iowrite64(sar, offset + DATA_SAR);
	iowrite64(dar, offset + DATA_DAR);
}

/*
 * Writes one data element of a DMA tranfser list via MMIO. This structure is used
 * to configure the DMA engine.
 */
static void write_xfer_list_link(enum ET_DMA_CHAN_ID id, uint32_t i,
				 uint64_t ptr, struct et_pci_dev *et_dev)
{
	void __iomem *offset = et_dev->iomem[IOMEM_R_DRCT_DRAM] +
			       XFER_LIST_OFFSETS[id] + i * XFER_NODE_BYTES;

	uint32_t ctrl_dword = CTRL_TCB | CTRL_LLP;

	iowrite32(ctrl_dword, offset + LINK_CTRL);
	iowrite64(ptr, offset + LINK_PTR);
}

static ssize_t et_dma_contig_buff(dma_addr_t buff, uint64_t soc_addr,
				  ssize_t count, enum ET_DMA_CHAN_ID id,
				  struct et_pci_dev *et_dev)
{
	ssize_t rv;
	struct dma_run_to_done_cmd_t run_msg = {
		.command_info.message_id = MBOX_DEVAPI_PRIVILEGED_MID_DMA_RUN_TO_DONE_CMD,
		.chan = (uint32_t)id
	};
	uint64_t src_addr, dest_addr;
	enum ET_DMA_STATE state;

	/* read: source is on host, dest is on SoC */
	if (id <= ET_DMA_CHAN_ID_READ_3) {
		src_addr = (uint64_t)buff;
		dest_addr = soc_addr;
	}
	/* write: source is one SoC, dest is on host */
	else {
		src_addr = soc_addr;
		dest_addr = (uint64_t)buff;
	}

	//Use MMIO to write a single-data-element xfer list
	write_xfer_list_data(id, 0, src_addr, dest_addr, count,
			     true /*interrupt*/, et_dev);
	write_xfer_list_link(id, 1,
			     DRAM_MEMMAP_BEGIN +
				     XFER_LIST_OFFSETS[id] /*circular link*/,
			     et_dev);

	//This code relies on mbox write doing a pcie read to make sure all of the
	//linked list node writes are done before DMA is started.

	//Signal the MM to start and supervise the DMA transefer
	rv = et_mbox_write(&et_dev->mbox_mm, &run_msg, sizeof(run_msg));
	if (rv < 0) {
		pr_err("mbox write errored\n");
		return rv;
	}
	if (rv != sizeof(run_msg)) {
		pr_err("mbox write didn't send all bytes\n");
		return -EIO;
	}

	//Wait for the DMA transfer to be done
	wait_event_interruptible(et_dev->dma_chans[id].wait_queue,
				 (state = et_dma_get_state(id, et_dev)) !=
					 ET_DMA_STATE_ACTIVE);
	
	if (state == ET_DMA_STATE_ABORTING) {
		//TODO: JIRA SW-1153: Signal DMA engine to abort
		pr_warn("DMA chan %d aborted!!!", id);
		return -EIO;
	}

	return count;
}

ssize_t et_dma_pull_from_user(const char __user *buf, size_t count, loff_t *pos,
			      enum ET_DMA_CHAN_ID id, struct et_pci_dev *et_dev)
{
	ssize_t rv;
	uint64_t soc_addr = (uint64_t)*pos;
	uint8_t *kern_buff;
	dma_addr_t kern_buff_addr;
	struct et_dma_chan *dma_chan;

	if (id < ET_DMA_CHAN_ID_READ_0 || id > ET_DMA_CHAN_ID_READ_3) {
		pr_err("invalid DMA read channel %d\n", id);
		return -EINVAL;
	}

	rv = et_dma_bounds_check(soc_addr, (uint64_t)count);
	if (rv != 0) {
		pr_err("bounds check failed\n");
		return rv;
	}

	if (count > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_chan = &et_dev->dma_chans[id];

	mutex_lock(&dma_chan->state_mutex);
	if (dma_chan->state != ET_DMA_STATE_IDLE) {
		dev_err(&et_dev->pdev->dev, "DMA chan %d not available, state %d\n",
			id, dma_chan->state);
		mutex_unlock(&dma_chan->state_mutex);
		return -EINVAL;
	}
	dma_chan->state = ET_DMA_STATE_ACTIVE;
	mutex_unlock(&dma_chan->state_mutex);

	//Allocate a contigious buffer to hold user's data
	//TODO: short-term: use the non-coherant API for perfomance
	//TODO: long-term: the buffer from user mode might be quite large (GB).
	//Copying the whole thing might be very bad, both for exec time and
	//memory pressure. Instead, pin/DMA it in little chunks at a time.
	kern_buff = dma_alloc_coherent(&et_dev->pdev->dev, count,
				       &kern_buff_addr, GFP_KERNEL);

	if (!kern_buff) {
		rv = -ENOMEM;
		pr_err("dma alloc failed\n");
		goto set_idle;
	}

	rv = copy_from_user(kern_buff, buf, count);
	if (rv != 0) {
		pr_err("Failed to copy from user\n");
		goto free_buff;
	}

	rv = et_dma_contig_buff(kern_buff_addr, soc_addr, count, id, et_dev);

free_buff:
	dma_free_coherent(&et_dev->pdev->dev, count, kern_buff, kern_buff_addr);

set_idle:
	mutex_lock(&dma_chan->state_mutex);
	dma_chan->state = dma_chan->state == ET_DMA_STATE_ABORTING ?
		ET_DMA_STATE_ABORTED:
		ET_DMA_STATE_IDLE;
	mutex_unlock(&dma_chan->state_mutex);

	if (rv > 0)
		*pos += rv;

	return rv;
}

ssize_t et_dma_push_to_user(char __user *buf, size_t count, loff_t *pos,
			    enum ET_DMA_CHAN_ID id, struct et_pci_dev *et_dev)
{
	ssize_t rv, dma_cnt;
	uint64_t soc_addr = (uint64_t)*pos;
	uint8_t *kern_buff;
	dma_addr_t kern_buff_addr;
	struct et_dma_chan *dma_chan;

	if (id < ET_DMA_CHAN_ID_WRITE_0 || id > ET_DMA_CHAN_ID_WRITE_3) {
		pr_err("invalid DMA write channel %d\n", id);
		return -EINVAL;
	}

	rv = et_dma_bounds_check(soc_addr, (uint64_t)count);
	if (rv != 0) {
		pr_err("bounds check failed\n");
		return rv;
	}

	if (count > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_chan = &et_dev->dma_chans[id];

	mutex_lock(&dma_chan->state_mutex);
	if (dma_chan->state != ET_DMA_STATE_IDLE) {
		dev_err(&et_dev->pdev->dev, "DMA chan %d already in use\n",
			id); //TODO: change all calls in this file to dev_err
		mutex_unlock(&dma_chan->state_mutex);
		return -EINVAL;
	}
	dma_chan->state = ET_DMA_STATE_ACTIVE;
	mutex_unlock(&dma_chan->state_mutex);

	//Allocate a contigious buffer to hold user's data
	//TODO: short-term: use the non-coherant API for perfomance
	//TODO: long-term: the buffer from user mode might be quite large (GB).
	//Copying the whole thing might be very bad, both for exec time and
	//memory pressure. Instead, pin/DMA it in little chunks at a time.
	kern_buff = dma_alloc_coherent(&et_dev->pdev->dev, count,
				       &kern_buff_addr, GFP_KERNEL);

	if (!kern_buff) {
		rv = -ENOMEM;
		pr_err("dma alloc failed\n");
		goto set_idle;
	}

	dma_cnt = et_dma_contig_buff(kern_buff_addr, soc_addr, count, id, et_dev);
	if (dma_cnt < 0) {
		rv = dma_cnt;
		goto free_buff;
	}

	rv = copy_to_user(buf, kern_buff, count);
	if (rv != 0) {
		pr_err("Failed to copy to user\n");
		goto free_buff;
	}

	rv = dma_cnt;

free_buff:
	dma_free_coherent(&et_dev->pdev->dev, count, kern_buff, kern_buff_addr);

set_idle:
	mutex_lock(&dma_chan->state_mutex);
	dma_chan->state = dma_chan->state == ET_DMA_STATE_ABORTING ?
		ET_DMA_STATE_ABORTED:
		ET_DMA_STATE_IDLE;
	mutex_unlock(&dma_chan->state_mutex);

	if (rv > 0)
		*pos += rv;

	return rv;
}
#endif

static inline void et_dma_free_coherent(struct et_dma_info *dma_info)
{
	if (dma_info)
		dma_free_coherent(&dma_info->pdev->dev, dma_info->size,
				  dma_info->kern_vaddr, dma_info->dma_addr);
}

static inline void *et_dma_alloc_coherent(struct et_dma_info *dma_info)
{
	if (dma_info)
		return dma_alloc_coherent(&dma_info->pdev->dev, dma_info->size,
					  &dma_info->dma_addr, GFP_KERNEL);
	return NULL;
}

// TODO: Enable with DMA scatter/gather and zero-copy implementation
#if 0
static ssize_t et_dma_pin_ubuf(struct et_dma_info *dma_info)
{
	size_t offs, i;
	ssize_t rv;

	if (!dma_info->usr_vaddr || dma_info->size == 0)
		return -EINVAL;

	/* determine space needed for page_list. */
	offs = offset_in_page(dma_info->usr_vaddr);
	dma_info->nr_pages = DIV_ROUND_UP(offs + dma_info->size, PAGE_SIZE);
	dma_info->page_list = kcalloc(dma_info->nr_pages,
				      sizeof(struct page *), GFP_KERNEL);
	if (!dma_info->page_list)
		return -ENOMEM;

	/* pin user pages in memory */
	rv = get_user_pages_fast((size_t)dma_info->usr_vaddr & PAGE_MASK,
				 dma_info->nr_pages,
				 dma_info->is_writable,
				 dma_info->page_list);
	if (rv < 0)
		goto fail_get_user_pages;

	if (rv < dma_info->nr_pages) {
		for (i = 0; i < rv; i++) {
			if (dma_info->page_list[i])
				put_page(dma_info->page_list[i]);
		}
		rv = -EFAULT;
		goto fail_get_user_pages;
	}

	return rv;

fail_get_user_pages:
	kfree(dma_info->page_list);
	return rv;
}

static void et_dma_unpin_ubuf(struct et_dma_info *dma_info)
{
	size_t i;

	if (dma_info && dma_info->page_list) {
		for (i = 0; i < dma_info->nr_pages; i++) {
			if (dma_info->is_writable)
				set_page_dirty_lock(dma_info->page_list[i]);
			if (dma_info->page_list[i])
				put_page(dma_info->page_list[i]);
		}
		kfree(dma_info->page_list);
	}
}
#endif

struct et_dma_info *et_dma_search_info(struct rb_root *root, u16 tag_id)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct et_dma_info *dma_info = rb_entry(node,
							struct et_dma_info,
							node);
		if (tag_id < dma_info->tag_id)
			node = node->rb_left;
		else if (tag_id > dma_info->tag_id)
			node = node->rb_right;
		else
			return dma_info;
	}
	return NULL;
}

bool et_dma_insert_info(struct rb_root *root, struct et_dma_info *dma_info)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct et_dma_info *this = rb_entry(*new, struct et_dma_info,
						    node);
		parent = *new;
		if (dma_info->tag_id < this->tag_id)
			new = &((*new)->rb_left);
		else if (dma_info->tag_id > this->tag_id)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&dma_info->node, parent, new);
	rb_insert_color(&dma_info->node, root);

	return true;
}

void et_dma_delete_info(struct rb_root *root, struct et_dma_info *dma_info)
{
	if (dma_info) {
		/* TODO JIRA SW-957: Uncomment when zero copy support is
		   available */
//		et_dma_unpin_ubuf(dma_info);
		et_dma_free_coherent(dma_info);

		rb_erase(&dma_info->node, root);
		kfree(dma_info);
	}
}

void et_dma_delete_all_info(struct rb_root *root)
{
	struct et_dma_info *dma_info;
	struct rb_node *node;

	for (node = rb_first(root); node;) {
		dma_info = rb_entry(node, struct et_dma_info, node);
		node = rb_next(node);

		/* TODO JIRA SW-957: Uncomment when zero copy support is
		   available */
//		et_dma_unpin_ubuf(dma_info);
		et_dma_free_coherent(dma_info);

		rb_erase(&dma_info->node, root);
		kfree(dma_info);
	}
}

ssize_t et_dma_write_to_device(struct et_pci_dev *et_dev, u16 queue_index,
			       struct device_ops_data_write_cmd_t *cmd,
			       size_t cmd_size)
{
	struct et_dma_info *dma_info;
	ssize_t rv;

	if (cmd->size > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_info = kmalloc(sizeof(*dma_info), GFP_KERNEL);
	if (!dma_info)
		return -ENOMEM;

	dma_info->tag_id = cmd->command_info.cmd_hdr.tag_id;
	dma_info->usr_vaddr = (void *)cmd->src_host_virt_addr;
	dma_info->pdev = et_dev->pdev;
	dma_info->size = cmd->size;
	dma_info->is_writable = false; /* readonly */
	dma_info->kern_vaddr = et_dma_alloc_coherent(dma_info);
	if (!dma_info->kern_vaddr) {
		pr_err("dma alloc failed\n");
		rv = -ENOMEM;
		goto error_free_dma_info;
	}

	/* TODO JIRA SW-957: Uncomment when zero copy support is available */
//	// Pin the user buffer to avoid swapping out of pages during DMA
//	// operations
//	rv = et_dma_pin_ubuf(dma_info);
//	if (rv < 0) {
//		pr_err("et_dma_pin_ubuf failed\n");
//		goto error_dma_free_coherent;
//	}

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	if (!et_dma_insert_info(&et_dev->ops.dma_rbtree, dma_info)) {
		pr_err("err: tag_id already exists\n");
		rv = -EINVAL;
		mutex_unlock(&et_dev->ops.dma_rbtree_mutex);
//		goto error_dma_unpin_ubuf;
		goto error_dma_free_coherent;
	}
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	rv = copy_from_user(dma_info->kern_vaddr, dma_info->usr_vaddr,
			    dma_info->size);
	if (rv != 0) {
		pr_err("Failed to copy from user\n");
		goto error_dma_delete_info;
	}

	cmd->src_host_phy_addr = dma_info->dma_addr;
	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		goto error_dma_delete_info;

	if (rv != cmd_size) {
		pr_err("vqueue write didn't send all bytes\n");
		rv = -EIO;
		goto error_dma_delete_info;
	}

	return rv;

error_dma_delete_info:
	et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);
	return rv;

/* TODO JIRA SW-957: Uncomment when zero copy support is available */
//error_dma_unpin_ubuf:
//	et_dma_unpin_ubuf(dma_info);

error_dma_free_coherent:
	et_dma_free_coherent(dma_info);

error_free_dma_info:
	kfree(dma_info);

	return rv;
}

ssize_t et_dma_read_from_device(struct et_pci_dev *et_dev, u16 queue_index,
				struct device_ops_data_read_cmd_t *cmd,
				size_t cmd_size)
{
	struct et_dma_info *dma_info;
	ssize_t rv;

	if (cmd->size > S32_MAX) {
		pr_err("Can't transfer more than 2GB at a time");
		return -EINVAL;
	}

	dma_info = kmalloc(sizeof(*dma_info), GFP_KERNEL);
	if (!dma_info)
		return -ENOMEM;

	dma_info->tag_id = cmd->command_info.cmd_hdr.tag_id;
	dma_info->usr_vaddr = (void *)cmd->dst_host_virt_addr;
	dma_info->pdev = et_dev->pdev;
	dma_info->size = cmd->size;
	dma_info->is_writable = true; /* writable */
	dma_info->kern_vaddr = et_dma_alloc_coherent(dma_info);
	if (!dma_info->kern_vaddr) {
		pr_err("dma alloc failed\n");
		rv = -ENOMEM;
		goto error_free_dma_info;
	}

	/* TODO JIRA SW-957: Uncomment when zero copy support is available */
//	// Pin the user buffer to avoid swapping out of pages during DMA
//	// operations
//	rv = et_dma_pin_ubuf(dma_info);
//	if (rv < 0) {
//		pr_err("et_dma_pin_ubuf failed\n");
//		goto error_dma_free_coherent;
//	}

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	if (!et_dma_insert_info(&et_dev->ops.dma_rbtree, dma_info)) {
		pr_err("err: tag_id already exists\n");
		rv = -EINVAL;
		mutex_unlock(&et_dev->ops.dma_rbtree_mutex);
//		goto error_dma_unpin_ubuf;
		goto error_dma_free_coherent;
	}
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	cmd->dst_host_phy_addr = dma_info->dma_addr;
	rv = et_squeue_push(et_dev->ops.sq_pptr[queue_index], cmd, cmd_size);
	if (rv < 0)
		goto error_dma_delete_info;

	if (rv != cmd_size) {
		pr_err("vqueue write didn't send all bytes\n");
		rv = -EIO;
		goto error_dma_delete_info;
	}

	return rv;

error_dma_delete_info:
	et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);
	return rv;

/* TODO JIRA SW-957: Uncomment when zero copy support is available */
//error_dma_unpin_ubuf:
//	et_dma_unpin_ubuf(dma_info);

error_dma_free_coherent:
	et_dma_free_coherent(dma_info);

error_free_dma_info:
	kfree(dma_info);

	return rv;
}

ssize_t et_dma_move_data(struct et_pci_dev *et_dev, u16 queue_index,
			 char __user *ucmd, size_t ucmd_size)
{
	void *kern_buf;
	struct cmn_header_t *header;
	ssize_t rv;

	if (ucmd_size < sizeof(struct cmn_header_t) || ucmd_size > U16_MAX) {
                pr_err("invalid cmd size: %ld", ucmd_size);
                return -EINVAL;
        }

	kern_buf = kzalloc(ucmd_size, GFP_KERNEL);

	if (!kern_buf)
		return -ENOMEM;

	rv = copy_from_user(kern_buf, ucmd, ucmd_size);
	if (rv) {
		pr_err("copy_from_user failed\n");
		rv = -ENOMEM;
		goto free_kern_buf;
	}

	header = (struct cmn_header_t *)kern_buf;

	if (header->msg_id == DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD) {
		if (ucmd_size < sizeof(struct device_ops_data_read_cmd_t)) {
			pr_err("Invalid DMA read cmd (size %ld)", ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_read_from_device(et_dev, queue_index, kern_buf,
					     ucmd_size);
	} else if (header->msg_id ==
		   DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD) {
		if (ucmd_size < sizeof(struct device_ops_data_write_cmd_t)) {
			pr_err("Invalid DMA write cmd (size %ld)", ucmd_size);
			rv = -EINVAL;
			goto free_kern_buf;
		}
		rv = et_dma_write_to_device(et_dev, queue_index, kern_buf,
					    ucmd_size);
	}

free_kern_buf:
	kfree(kern_buf);

	return rv;
}
