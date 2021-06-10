// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "et_dma.h"
#include "et_event_handler.h"
#include "et_io.h"
#include "et_pci_dev.h"
#include "et_vqueue.h"

static struct et_msg_node *create_msg_node(u32 msg_size)
{
	struct et_msg_node *new_node;
	//Build node
	new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);

	if (IS_ERR(new_node)) {
		panic("Failed to allocate msg node, error %ld\n",
		      PTR_ERR(new_node));
		return NULL;
	}

	new_node->msg = kmalloc(msg_size, GFP_KERNEL);

	if (IS_ERR(new_node->msg)) {
		panic("Failed to allocate msg buffer, error %ld\n",
		      PTR_ERR(new_node->msg));
		return NULL;
	}

	new_node->msg_size = msg_size;

	return new_node;
}

static void enqueue_msg_node(struct et_cqueue *cq, struct et_msg_node *msg)
{
	mutex_lock(&cq->msg_list_mutex);
	list_add_tail(&msg->list, &cq->msg_list);
	mutex_unlock(&cq->msg_list_mutex);
}

struct et_msg_node *et_dequeue_msg_node(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node, list);
	if (msg)
		list_del(&msg->list);

	mutex_unlock(&cq->msg_list_mutex);

	return msg;
}

static void destroy_msg_node(struct et_msg_node *node)
{
	if (node) {
		kfree(node->msg);
		kfree(node);
	}
}

void et_destroy_msg_list(struct et_cqueue *cq)
{
	struct list_head *pos, *next;
	struct et_msg_node *node;
	int count = 0;

	mutex_lock(&cq->msg_list_mutex);
	list_for_each_safe (pos, next, &cq->msg_list) {
		node = list_entry(pos, struct et_msg_node, list);
		list_del(pos);
		destroy_msg_node(node);
		count++;
	}
	mutex_unlock(&cq->msg_list_mutex);

	if (count)
		pr_warn("Discarded (%d) CQ user messages", count);
}

bool et_cqueue_msg_available(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node, list);
	mutex_unlock(&cq->msg_list_mutex);

	return !!(msg);
}

static irqreturn_t et_pcie_isr(int irq, void *cq_id)
{
	struct et_cqueue *cq = (struct et_cqueue *)cq_id;

	queue_work(cq->vq_common->workqueue, &cq->isr_work);

	return IRQ_HANDLED;
}

static void et_isr_work(struct work_struct *work)
{
	struct et_cqueue *cq = container_of(work, struct et_cqueue, isr_work);

	// TODO: Move following to separate ISR for SQ
	wake_up_interruptible(&cq->vq_common->waitqueue);

	et_cqueue_isr_bottom(cq);
}

static ssize_t et_squeue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i;
	struct et_vq_common *vq_common;
	struct et_squeue **sq_pptr;
	struct et_mapped_region *vq_region;
	u8 *mem, __iomem *sq_baseaddr;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
	} else {
		vq_common = &et_dev->ops.vq_common;
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	}

	mem = kmalloc_array(vq_common->dir_vq.sq_count,
			    sizeof(*sq_pptr) + sizeof(**sq_pptr),
			    GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	sq_pptr = (struct et_squeue **)mem;
	mem += vq_common->dir_vq.sq_count * sizeof(*sq_pptr);

	sq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
		      vq_common->dir_vq.sq_offset;
	for (i = 0; i < vq_common->dir_vq.sq_count; i++) {
		sq_pptr[i] = (struct et_squeue *)mem;
		mem += sizeof(**sq_pptr);

		sq_pptr[i]->index = i;
		sq_pptr[i]->vq_common = vq_common;
		sq_pptr[i]->cb_mem =
			(struct et_circbuffer __iomem *)sq_baseaddr;
		et_ioread(sq_pptr[i]->cb_mem,
			  0,
			  (u8 *)&sq_pptr[i]->cb,
			  sizeof(sq_pptr[i]->cb));
		sq_baseaddr += vq_common->dir_vq.per_sq_size;

		mutex_init(&sq_pptr[i]->push_mutex);
		atomic_set(&sq_pptr[i]->sq_threshold,
			   (vq_common->dir_vq.per_sq_size -
			    sizeof(struct et_circbuffer)) /
				   4);
	}

	if (is_mgmt)
		et_dev->mgmt.sq_pptr = sq_pptr;
	else
		et_dev->ops.sq_pptr = sq_pptr;

	return 0;
}

static ssize_t et_cqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	char irq_name[16];
	ssize_t rv;
	u32 i, irq_cnt_init;
	struct et_vq_common *vq_common;
	struct et_cqueue **cq_pptr;
	struct et_mapped_region *vq_region;
	u8 *mem, __iomem *cq_baseaddr;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
	} else {
		vq_common = &et_dev->ops.vq_common;
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	}

	mem = kmalloc_array(vq_common->dir_vq.cq_count,
			    sizeof(*cq_pptr) + sizeof(**cq_pptr),
			    GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	cq_pptr = (struct et_cqueue **)mem;
	mem += vq_common->dir_vq.cq_count * sizeof(*cq_pptr);

	cq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
		      vq_common->dir_vq.cq_offset;

	for (i = 0, irq_cnt_init = 0; i < vq_common->dir_vq.cq_count;
	     i++, irq_cnt_init++) {
		cq_pptr[i] = (struct et_cqueue *)mem;
		mem += sizeof(**cq_pptr);

		cq_pptr[i]->index = i;
		cq_pptr[i]->vq_common = vq_common;
		cq_pptr[i]->cb_mem =
			(struct et_circbuffer __iomem *)cq_baseaddr;
		et_ioread(cq_pptr[i]->cb_mem,
			  0,
			  (u8 *)&cq_pptr[i]->cb,
			  sizeof(cq_pptr[i]->cb));
		cq_baseaddr += vq_common->dir_vq.per_cq_size;

		mutex_init(&cq_pptr[i]->pop_mutex);
		INIT_LIST_HEAD(&cq_pptr[i]->msg_list);
		mutex_init(&cq_pptr[i]->msg_list_mutex);

		INIT_WORK(&cq_pptr[i]->isr_work, et_isr_work);

		snprintf(irq_name,
			 sizeof(irq_name),
			 "irq_%s_cq_%d",
			 (is_mgmt) ? "mgmt" : "ops",
			 i);
		rv = request_irq(pci_irq_vector(et_dev->pdev,
						vq_common->vec_idx_offset + i),
				 et_pcie_isr,
				 0,
				 irq_name,
				 (void *)cq_pptr[i]);
		if (rv) {
			dev_err(&et_dev->pdev->dev, "request irq failed\n");
			goto error_free_irq;
		}
	}

	if (is_mgmt)
		et_dev->mgmt.cq_pptr = cq_pptr;
	else
		et_dev->ops.cq_pptr = cq_pptr;

	return 0;

error_free_irq:
	for (i = 0; i < irq_cnt_init; i++)
		free_irq(pci_irq_vector(et_dev->pdev,
					vq_common->vec_idx_offset + i),
			 (void *)cq_pptr[i]);

	kfree(cq_pptr);

	return rv;
}

static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t rv;
	struct et_vq_common *vq_common;
	struct et_mapped_region *intrpt_region =
		&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG];
	char wq_name[32];

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;

		// Initialize Mgmt device workqueue
		snprintf(wq_name,
			 sizeof(wq_name),
			 "%s_mgmt_wq%d",
			 dev_name(&et_dev->pdev->dev),
			 et_dev->dev_index);
		vq_common->workqueue = create_singlethread_workqueue(wq_name);
		if (!vq_common->workqueue)
			return -ENOMEM;
	} else {
		vq_common = &et_dev->ops.vq_common;

		// Initialize Ops device workqueue
		snprintf(wq_name,
			 sizeof(wq_name),
			 "%s_ops_wq%d",
			 dev_name(&et_dev->pdev->dev),
			 et_dev->dev_index);
		vq_common->workqueue = create_singlethread_workqueue(wq_name);
		if (!vq_common->workqueue)
			return -ENOMEM;
	}

	// Set interrupt address
	if (!intrpt_region->is_valid) {
		rv = -EINVAL;
		goto error_destroy_workqueue;
	}

	vq_common->intrpt_addr = (u8 __iomem *)intrpt_region->mapped_baseaddr +
				 vq_common->dir_vq.intrpt_trg_offset;

	if (et_dev->used_irq_vecs + vq_common->dir_vq.cq_count >
	    et_dev->num_irq_vecs) {
		dev_err(&et_dev->pdev->dev, "VQ: not enough vecs allocated\n");
		rv = -EINVAL;
		goto error_destroy_workqueue;
	}
	vq_common->vec_idx_offset = et_dev->used_irq_vecs;

	bitmap_zero(vq_common->sq_bitmap, ET_MAX_QUEUES);
	bitmap_zero(vq_common->cq_bitmap, ET_MAX_QUEUES);
	init_waitqueue_head(&vq_common->waitqueue);
	vq_common->aborting = false;
	spin_lock_init(&vq_common->abort_lock);
	vq_common->pdev = et_dev->pdev;

	rv = et_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_destroy_workqueue;

	rv = et_cqueue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_squeue_destroy_all;

	et_dev->used_irq_vecs += vq_common->dir_vq.cq_count;

	return rv;

error_squeue_destroy_all:
	et_squeue_destroy_all(et_dev, is_mgmt);

error_destroy_workqueue:
	destroy_workqueue(vq_common->workqueue);
	return rv;
}

static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_vq_common *vq_common;
	struct et_squeue **sq_pptr;
	ssize_t i;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;
		sq_pptr = et_dev->mgmt.sq_pptr;
	} else {
		vq_common = &et_dev->ops.vq_common;
		sq_pptr = et_dev->ops.sq_pptr;
	}

	for (i = 0; i < vq_common->dir_vq.sq_count; i++) {
		mutex_destroy(&sq_pptr[i]->push_mutex);
		sq_pptr[i]->cb_mem = NULL;
		sq_pptr[i]->vq_common = NULL;
		sq_pptr[i] = NULL;
	}

	kfree(sq_pptr);
}

static void et_cqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_vq_common *vq_common;
	struct et_cqueue **cq_pptr;
	u32 i;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;
		cq_pptr = et_dev->mgmt.cq_pptr;
	} else {
		vq_common = &et_dev->ops.vq_common;
		cq_pptr = et_dev->ops.cq_pptr;
	}

	for (i = 0; i < vq_common->dir_vq.cq_count; i++) {
		free_irq(pci_irq_vector(et_dev->pdev,
					vq_common->vec_idx_offset + i),
			 (void *)cq_pptr[i]);
		cancel_work_sync(&cq_pptr[i]->isr_work);
		mutex_destroy(&cq_pptr[i]->pop_mutex);
		et_destroy_msg_list(cq_pptr[i]);
		mutex_destroy(&cq_pptr[i]->msg_list_mutex);
		cq_pptr[i]->cb_mem = NULL;
		cq_pptr[i]->vq_common = NULL;
		cq_pptr[i] = NULL;
	}

	kfree(cq_pptr);
}

void et_vqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_vq_common *vq_common;
	unsigned long flags;

	if (is_mgmt)
		vq_common = &et_dev->mgmt.vq_common;
	else
		vq_common = &et_dev->ops.vq_common;

	spin_lock_irqsave(&vq_common->abort_lock, flags);
	vq_common->aborting = true;
	spin_unlock_irqrestore(&vq_common->abort_lock, flags);

	wake_up_interruptible_all(&vq_common->waitqueue);

	et_cqueue_destroy_all(et_dev, is_mgmt);
	et_squeue_destroy_all(et_dev, is_mgmt);

	et_dev->used_irq_vecs -= vq_common->dir_vq.cq_count;
	destroy_workqueue(vq_common->workqueue);
}

static inline void interrupt_device(struct et_squeue *sq)
{
	switch (sq->vq_common->dir_vq.intrpt_trg_size) {
	case 1:
		iowrite8(sq->vq_common->dir_vq.intrpt_id,
			 sq->vq_common->intrpt_addr);
		break;
	case 2:
		iowrite16(sq->vq_common->dir_vq.intrpt_id,
			  sq->vq_common->intrpt_addr);
		break;
	case 4:
		iowrite32(sq->vq_common->dir_vq.intrpt_id,
			  sq->vq_common->intrpt_addr);
		break;
	case 8:
		iowrite64(sq->vq_common->dir_vq.intrpt_id,
			  sq->vq_common->intrpt_addr);
	}
}

ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count)
{
	struct cmn_header_t *header = buf;
	ssize_t rv = count;

	if (count < sizeof(*header)) {
		pr_err("VQ[%d]: size too small: %ld", sq->index, count);
		return -EINVAL;
	}

	if (header->size > count) {
		pr_err("VQ[%d]: header contains invalid cmd size", sq->index);
		return -EINVAL;
	}

	mutex_lock(&sq->push_mutex);

	if (!et_circbuffer_push(&sq->cb,
				sq->cb_mem,
				buf,
				header->size,
				ET_CB_SYNC_FOR_HOST | ET_CB_SYNC_FOR_DEVICE)) {
		// Full; no room for message, returning EAGAIN
		rv = -EAGAIN;
		goto update_sq_bitmap;
	}

	// Inform device that message has been pushed to SQ
	interrupt_device(sq);

update_sq_bitmap:
	mutex_unlock(&sq->push_mutex);

	// Update sq_bitmap
	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold))
		set_bit(sq->index, sq->vq_common->sq_bitmap);
	else
		clear_bit(sq->index, sq->vq_common->sq_bitmap);

	return rv;
}

ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev,
				 bool is_mgmt,
				 u16 sq_index,
				 const char __user *ubuf,
				 size_t count)
{
	struct et_squeue *sq;
	u8 *kern_buf;
	ssize_t rv;

	if (is_mgmt)
		sq = et_dev->mgmt.sq_pptr[sq_index];
	else
		sq = et_dev->ops.sq_pptr[sq_index];

	if (!count || count > U16_MAX) {
		pr_err("invalid message size: %ld", count);
		return -EINVAL;
	}

	kern_buf = kzalloc(count, GFP_KERNEL);

	if (!kern_buf)
		return -ENOMEM;

	rv = copy_from_user(kern_buf, ubuf, count);
	if (rv) {
		pr_err("copy_from_user failed\n");
		rv = -ENOMEM;
		goto error;
	}

	rv = et_squeue_push(sq, kern_buf, count);

error:
	kfree(kern_buf);

	return rv;
}

static inline void et_squeue_sync_cb_for_host(struct et_squeue *sq)
{
	u64 head_local;

	mutex_lock(&sq->push_mutex);
	head_local = sq->cb.head;
	et_ioread(sq->cb_mem, 0, (u8 *)&sq->cb, sizeof(sq->cb));

	if (head_local != sq->cb.head)
		pr_err("SQ sync: head mismatched, head_local: %lld, head_remote: %lld",
		       head_local,
		       sq->cb.head);

	mutex_unlock(&sq->push_mutex);
}

bool et_squeue_event_available(struct et_squeue *sq)
{
	if (!sq)
		return false;

	// Sync SQ circbuffer
	if (test_bit(sq->index, sq->vq_common->sq_bitmap))
		return true;

	et_squeue_sync_cb_for_host(sq);

	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold)) {
		set_bit(sq->index, sq->vq_common->sq_bitmap);
		return true;
	}

	return false;
}

static ssize_t free_dma_kernel_entry(struct et_pci_dev *et_dev,
				     bool is_mgmt,
				     struct et_msg_node *msg)
{
	struct cmn_header_t *header = NULL;
	struct device_ops_data_read_rsp_t *read_rsp = NULL;
	struct et_dma_info *dma_info = NULL;
	ssize_t rv = 0;

	// No DMA kernel entry for mgmt_dev
	if (is_mgmt)
		return 0;

	if (!msg || !msg->msg)
		// empty msg node, try again
		return -EAGAIN;

	if (msg->msg_size < sizeof(*header))
		return -EINVAL;

	header = (struct cmn_header_t *)msg->msg;
	if (header->msg_id != DEV_OPS_API_MID_DATA_READ_RSP &&
	    header->msg_id != DEV_OPS_API_MID_DATA_WRITE_RSP)
		return 0;

	mutex_lock(&et_dev->ops.dma_rbtree_mutex);
	dma_info = et_dma_search_info(&et_dev->ops.dma_rbtree, header->tag_id);
	if (!dma_info) {
		pr_err("DMA response kernel entry not found!");
		rv = -EINVAL;
		goto unlock_rbtree_mutex;
	}

	// Copy the DMA data to user buffer if it is data read response with
	// complete status otherwise only remove the kernel entry
	if (header->msg_id == DEV_OPS_API_MID_DATA_READ_RSP) {
		if (header->size < (sizeof(*read_rsp) - sizeof(*header))) {
			pr_err("Corrupted DATA read response received!");
			rv = -EINVAL;
			goto dma_delete_info;
		}

		read_rsp = (struct device_ops_data_read_rsp_t *)header;
		if (read_rsp->status != DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
			// No DMA data to copy to user
			rv = 0;
			goto dma_delete_info;
		}

		if (copy_to_user(dma_info->usr_vaddr,
				 dma_info->kern_vaddr,
				 dma_info->size)) {
			pr_err("failed to copy DMA buffer\n");
			rv = -EINVAL;
			goto dma_delete_info;
		}
		rv = dma_info->size;
	}

dma_delete_info:
	et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);

unlock_rbtree_mutex:
	mutex_unlock(&et_dev->ops.dma_rbtree_mutex);

	return rv;
}

ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev,
			       bool is_mgmt,
			       u16 cq_index,
			       char __user *ubuf,
			       size_t count)
{
	struct et_cqueue *cq;
	struct et_msg_node *msg = NULL;
	ssize_t rv;

	if (is_mgmt)
		cq = et_dev->mgmt.cq_pptr[cq_index];
	else
		cq = et_dev->ops.cq_pptr[cq_index];

	if (cq->vq_common->aborting)
		return -EINTR;

	if (!ubuf || !count)
		return -EINVAL;

	msg = et_dequeue_msg_node(cq);
	if (!msg || !(msg->msg)) {
		// Empty; no message to POP, returning EAGAIN
		rv = -EAGAIN;
		goto update_cq_bitmap;
	}

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		rv = -ENOMEM;
		goto update_cq_bitmap;
	}

	rv = free_dma_kernel_entry(et_dev, is_mgmt, msg);
	if (rv < 0)
		goto update_cq_bitmap;

	if (copy_to_user(ubuf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		rv = -ENOMEM;
		goto update_cq_bitmap;
	}

	rv = msg->msg_size;

update_cq_bitmap:
	// Update cq_bitmap
	if (et_cqueue_msg_available(cq))
		set_bit(cq->index, cq->vq_common->cq_bitmap);
	else
		clear_bit(cq->index, cq->vq_common->cq_bitmap);

	return rv;
}

ssize_t et_cqueue_pop(struct et_cqueue *cq, bool sync_for_host)
{
	struct cmn_header_t header;
	struct et_msg_node *msg_node;
	ssize_t rv;

	mutex_lock(&cq->pop_mutex);

	// Read the message header
	if (!et_circbuffer_pop(&cq->cb,
			       cq->cb_mem,
			       (u8 *)&header,
			       sizeof(header),
			       (sync_for_host) ? ET_CB_SYNC_FOR_HOST : 0)) {
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	// If the size is invalid, the message buffer is corrupt and the
	// system is in a bad state. This should never happen.
	// TODO: Add some recovery mechanism
	if (!header.size)
		panic("CQ corrupt: invalid size");

	if (header.msg_id >= DEV_MGMT_API_MID_EVENTS_BEGIN &&
	    header.msg_id <= DEV_MGMT_API_MID_EVENTS_END) {
		rv = et_handle_device_event(cq, &header);
		mutex_unlock(&cq->pop_mutex);
		return rv;
	}

	// Message is for user mode. Save it off.
	msg_node = create_msg_node(header.size + sizeof(header));
	if (!msg_node) {
		rv = -ENOMEM;
		goto error_unlock_mutex;
	}

	memcpy(msg_node->msg, (u8 *)&header, sizeof(header));

	// MMIO msg payload into node memory
	if (!et_circbuffer_pop(&cq->cb,
			       cq->cb_mem,
			       (u8 *)msg_node->msg + sizeof(header),
			       header.size,
			       ET_CB_SYNC_FOR_DEVICE)) {
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	mutex_unlock(&cq->pop_mutex);

	// Enqueue msg node to user msg_list of CQ
	enqueue_msg_node(cq, msg_node);

	set_bit(cq->index, cq->vq_common->cq_bitmap);
	wake_up_interruptible(&cq->vq_common->waitqueue);

	return header.size;

error_unlock_mutex:
	mutex_unlock(&cq->pop_mutex);

	return rv;
}

static inline void et_cqueue_sync_cb_for_host(struct et_cqueue *cq)
{
	u64 tail_local;

	mutex_lock(&cq->pop_mutex);
	tail_local = cq->cb.tail;
	et_ioread(cq->cb_mem, 0, (u8 *)&cq->cb, sizeof(cq->cb));

	if (tail_local != cq->cb.tail)
		pr_err("CQ sync: tail mismatched, tail_local: %lld, tail_remote: %lld",
		       tail_local,
		       cq->cb.tail);

	mutex_unlock(&cq->pop_mutex);
}

bool et_cqueue_event_available(struct et_cqueue *cq)
{
	if (!cq)
		return false;

	// Sync CQ circbuffer
	if (test_bit(cq->index, cq->vq_common->cq_bitmap))
		return true;

	et_cqueue_sync_cb_for_host(cq);

	if (et_cqueue_msg_available(cq)) {
		set_bit(cq->index, cq->vq_common->cq_bitmap);
		return true;
	}

	return false;
}

/*
 * Handles vqueue IRQ. The vqueue IRQ signals a a new message in the CQ.
 *
 * This method handles CQ messages for the kernel immediatley, and saves
 * off messages for user mode to be consumed later.
 *
 * User mode messages must not block kernel messages from being processed
 * (e.g if the first msg in the vqueue is for the user and the second is for
 * the kernel, the kernel message should not be stuck in line behind the user
 * message).
 *
 * This method must be tolerant of spurious IRQs (no new msg), and taking an
 * IRQ while messages are still in filght.
 *
 * Reasons it may fire:
 *
 * - The host sent a msg to this vqueue
 *
 * - The host sent a msg to another vqueue, and MSI
 *   multivector support is not available (IRQ is spurious for this vqueue)
 *
 * - The host sent two (or more) messages and two (or more) IRQs, but the ISR
 *   handeled multiple messages in one pass, (follow-on IRQs should be ignored)
 *
 *   Another version of this: the ISR sees the data for the first message, and
 *   only a portion of the data for the second message (rest of data and second
 *   IRQ still in flight)
 *
 * - Perodic wakeup fired (incase IRQs missed). There may be a state update or
 *   msg, there may be a message in flight (should take no action and wait for
 *   next IRQ), or there may be no changes (IRQ is spurious)
 */
void et_cqueue_isr_bottom(struct et_cqueue *cq)
{
	bool sync_for_host = true;

	// Handle all pending messages in the cqueue
	while (et_cqueue_pop(cq, sync_for_host) > 0) {
		// Only sync `circbuffer` the first time
		if (sync_for_host)
			sync_for_host = false;
	}
}
