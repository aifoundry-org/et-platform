// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "et_dma.h"
#include "et_io.h"
#include "et_vqueue.h"
#include "et_pci_dev.h"
#include "et_mgmt_dir.h"
#include "et_ops_dir.h"

/*
 * Timeout is 250ms. Picked because it's unlikley the driver will miss an IRQ,
 * so this is a contigency and does not need to be checked often.
 */
#define MISSED_IRQ_TIMEOUT (HZ / 4)

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

static struct et_msg_node *dequeue_msg_node(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node,
				       list);
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

static void destroy_msg_list(struct et_cqueue *cq)
{
	struct list_head *pos, *next;
	struct et_msg_node *node;

	mutex_lock(&cq->msg_list_mutex);
	list_for_each_safe(pos, next, &cq->msg_list) {
		node = list_entry(pos, struct et_msg_node, list);
		list_del(pos);
		destroy_msg_node(node);
	}
	mutex_unlock(&cq->msg_list_mutex);
}

bool et_cqueue_msg_available(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node,
				       list);
	mutex_unlock(&cq->msg_list_mutex);

	return !!(msg);
}

// TODO SW-4210: Uncomment when MSIx is enabled
#if 0
static irqreturn_t et_pcie_isr(int irq, void *cq_id)
{
	struct et_cqueue *cq = (struct et_cqueue *)cq_id;

	queue_work(cq->vq_common->workqueue, &cq->isr_work);

	return IRQ_HANDLED;
}

static void et_isr_work(struct work_struct *work)
{
	struct et_cqueue *cq = container_of(work, struct et_cqueue, isr_work);

	et_cqueue_isr_bottom(cq);
}
#endif

static ssize_t et_squeue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i;
	struct et_vq_common *vq_common;
	struct et_squeue **sq_pptr;
	u8 *mem, *sq_baseaddr;

	if (is_mgmt)
		vq_common = &et_dev->mgmt.vq_common;
	else
		vq_common = &et_dev->ops.vq_common;

	mem = kmalloc_array(vq_common->sq_count,
			    sizeof(*sq_pptr) + sizeof(**sq_pptr), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	sq_pptr = (struct et_squeue **)mem;
	mem += vq_common->sq_count * sizeof(*sq_pptr);

	sq_baseaddr = vq_common->mapped_baseaddr;
	for (i = 0; i < vq_common->sq_count; i++) {
		sq_pptr[i] = (struct et_squeue *)mem;
		mem += sizeof(**sq_pptr);

		sq_pptr[i]->index = i;
		sq_pptr[i]->cb = (struct et_circbuffer *)sq_baseaddr;
		sq_baseaddr += vq_common->sq_size;

		mutex_init(&sq_pptr[i]->push_mutex);
		atomic_set(&sq_pptr[i]->sq_threshold, (vq_common->sq_size -
			   sizeof(struct et_circbuffer)) / 4);
		sq_pptr[i]->vq_common = vq_common;
	}

	if (is_mgmt)
		et_dev->mgmt.sq_pptr = sq_pptr;
	else
		et_dev->ops.sq_pptr = sq_pptr;

	return 0;
}

static ssize_t et_cqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	// TODO SW-4210: Uncomment when MSIx is enabled
//	char irq_name[16];
//	ssize_t rv;
//	u32 i, irq_cnt_init;
	u32 i;
	struct et_vq_common *vq_common;
	struct et_cqueue **cq_pptr;
	u8 *mem, *cq_baseaddr;

	if (is_mgmt)
		vq_common = &et_dev->mgmt.vq_common;
	else
		vq_common = &et_dev->ops.vq_common;

	mem = kmalloc_array(vq_common->cq_count,
			    sizeof(*cq_pptr) + sizeof(**cq_pptr), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	cq_pptr = (struct et_cqueue **)mem;
	mem += vq_common->cq_count * sizeof(*cq_pptr);

	cq_baseaddr = (u8 *)vq_common->mapped_baseaddr +
		vq_common->sq_count * vq_common->sq_size;

	// TODO SW-4210: Uncomment when MSIx is enabled
//	for (i = 0, irq_cnt_init = 0; i < vq_common->cq_count; i++,
//	     irq_cnt_init++) {
	for (i = 0; i < vq_common->cq_count; i++) {
		cq_pptr[i] = (struct et_cqueue *)mem;
		mem += sizeof(**cq_pptr);

		cq_pptr[i]->index = i;
		cq_pptr[i]->cb = (struct et_circbuffer *)cq_baseaddr;
		cq_baseaddr += vq_common->cq_size;

		mutex_init(&cq_pptr[i]->pop_mutex);
		INIT_LIST_HEAD(&cq_pptr[i]->msg_list);
		mutex_init(&cq_pptr[i]->msg_list_mutex);

		// TODO SW-4210: Uncomment when MSIx is enabled
//		snprintf(irq_name, sizeof(irq_name), "irq_%s_cq_%d",
//			 (is_mgmt) ? "mgmt" : "ops", i);
//		rv = request_irq(pci_irq_vector(et_dev->pdev,
//						vq_common->vec_idx_offset + i),
//				 et_pcie_isr, 0, irq_name, (void *)cq_pptr[i]);
//		if (rv) {
//			dev_err(&et_dev->pdev->dev, "request irq failed\n");
//			goto error_free_irq;
//		}
//		INIT_WORK(&cq_pptr[i]->isr_work, et_isr_work);
		cq_pptr[i]->vq_common = vq_common;
	}

	if (is_mgmt)
		et_dev->mgmt.cq_pptr = cq_pptr;
	else
		et_dev->ops.cq_pptr = cq_pptr;

	return 0;

// TODO SW-4210: Uncomment when MSIx is enabled
//error_free_irq:
//	for (i = 0; i < irq_cnt_init; i++)
//		free_irq(pci_irq_vector(et_dev->pdev,
//			 vq_common->vec_idx_offset + i), (void *)cq_pptr[i]);
//
//	kfree(cq_pptr);
//
//	return rv;
}

static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t rv;
	struct et_bar_mapping bm_info;
	struct et_mgmt_dir *dir_mgmt;
	struct et_mgmt_vqueue vq_mgmt;
	struct et_ops_dir *dir_ops;
	struct et_ops_vqueue vq_ops;
	struct et_vq_common *vq_common;
	char wq_name[32];

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_common;
		dir_mgmt = (struct et_mgmt_dir *)et_dev->mgmt.dir;

		rv = (s32)ioread32(&dir_mgmt->status);
		if (rv < MGMT_BOOT_STATUS_DEV_READY) {
			dev_err(&et_dev->pdev->dev,
				"Mgmt device DIRs not ready, status: %ld\n",
				rv);
			return -EBUSY;
		}

		// Set Mgmt device interrupt address
		if (!et_dev->r_pu_trg_pcie)
			return -EINVAL;
		vq_common->interrupt_addr = et_dev->r_pu_trg_pcie
			+ ioread32(&dir_mgmt->intrpt_trg_offset);

		// Perform optimized read of VQ fields from DIRs
		et_ioread(dir_mgmt, offsetof(struct et_mgmt_dir, vq_mgmt),
			  (u8 *)&vq_mgmt, sizeof(vq_mgmt));

		// Get Mgmt device SQ and CQ count from DIR
		vq_common->sq_count = vq_mgmt.sq_count;
		vq_common->cq_count = vq_mgmt.cq_count;

		// Get Mgmt device SQ and CQ size from DIR
		vq_common->sq_size = vq_mgmt.per_sq_size;
		vq_common->cq_size = vq_mgmt.per_cq_size;

		// Initialize Mgmt device workqueue
		snprintf(wq_name, sizeof(wq_name), "%s_mgmt_wq%d",
			 dev_name(&et_dev->pdev->dev), et_dev->dev_index);
		vq_common->workqueue = create_singlethread_workqueue(wq_name);
		if (!vq_common->workqueue)
			return -ENOMEM;

		// Discover bar mapping information for Mgmt device VQs
		bm_info.bar			= vq_mgmt.bar;
		bm_info.bar_offset		= vq_mgmt.sq_offset;
		bm_info.size			= vq_mgmt.bar_size;
	} else {
		vq_common = &et_dev->ops.vq_common;
		dir_ops = (struct et_ops_dir *)et_dev->ops.dir;

		rv = (s32)ioread32(&dir_ops->status);
		if (rv < OPS_BOOT_STATUS_MM_READY) {
			dev_err(&et_dev->pdev->dev,
				"Ops device DIR not ready, status: %ld\n",
				rv);
			return -EBUSY;
		}

		// Set Mgmt device interrupt address
		if (!et_dev->r_pu_trg_pcie)
			return -EINVAL;
		vq_common->interrupt_addr = et_dev->r_pu_trg_pcie
			+ ioread32(&dir_ops->intrpt_trg_offset);

		// Perform optimized read of VQ fields from DIRs
		et_ioread(dir_ops, offsetof(struct et_ops_dir, vq_ops),
			  (u8 *)&vq_ops, sizeof(vq_ops));

		// Get Ops device SQ and CQ count from DIR
		vq_common->sq_count = vq_ops.sq_count;
		vq_common->cq_count = vq_ops.cq_count;

		// Get Ops device SQ and CQ size from DIR
		vq_common->sq_size = vq_ops.per_sq_size;
		vq_common->cq_size = vq_ops.per_cq_size;

		// Initialize Ops device workqueue
		snprintf(wq_name, sizeof(wq_name), "%s_ops_wq%d",
			 dev_name(&et_dev->pdev->dev), et_dev->dev_index);
		vq_common->workqueue = create_singlethread_workqueue(wq_name);
		if (!vq_common->workqueue)
			return -ENOMEM;

		// Discover bar mapping information for Ops device VQs
		bm_info.bar			= vq_ops.bar;
		bm_info.bar_offset		= vq_ops.sq_offset;
		bm_info.size			= vq_ops.bar_size;
	}

	// Map virtual queues region
	rv = et_map_bar(et_dev, &bm_info, &vq_common->mapped_baseaddr);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "VQ region mapping failed\n");
		goto error_destroy_workqueue;
	}

	// TODO SW-4210: Uncomment when MSIx is enabled
//	if (et_dev->used_irq_vecs + vq_common->cq_count >
//	    et_dev->num_irq_vecs) {
//		dev_err(&et_dev->pdev->dev,
//			"VQ: not enough vecs allocated\n");
//		rv = -EINVAL;
//		goto error_unmap_vq_bar_region;
//	}
//	vq_common->vec_idx_offset = et_dev->used_irq_vecs;

	bitmap_zero(vq_common->sq_bitmap, ET_MAX_QUEUES);
	bitmap_zero(vq_common->cq_bitmap, ET_MAX_QUEUES);
	init_waitqueue_head(&vq_common->waitqueue);
	vq_common->aborting = false;
	spin_lock_init(&vq_common->abort_lock);

	rv = et_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_unmap_vq_bar_region;

	rv = et_cqueue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_squeue_destroy_all;

	// TODO SW-4210: Uncomment when MSIx is enabled
//	et_dev->used_irq_vecs += vq_common->cq_count;

	return rv;

error_squeue_destroy_all:
	et_squeue_destroy_all(et_dev, is_mgmt);

error_unmap_vq_bar_region:
	et_unmap_bar(vq_common->mapped_baseaddr);

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

	for (i = 0; i < vq_common->sq_count; i++) {
		mutex_destroy(&sq_pptr[i]->push_mutex);
		sq_pptr[i]->cb = NULL;
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

	for (i = 0; i < vq_common->cq_count; i++) {
		// TODO SW-4210: Uncomment when MSIx is enabled
//		cancel_work_sync(&cq_pptr[i]->isr_work);
//		free_irq(pci_irq_vector(et_dev->pdev,
//					vq_common->vec_idx_offset + i),
//			 (void *)cq_pptr[i]);
		mutex_destroy(&cq_pptr[i]->pop_mutex);
		destroy_msg_list(cq_pptr[i]);
		mutex_destroy(&cq_pptr[i]->msg_list_mutex);
		cq_pptr[i]->cb = NULL;
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

	et_squeue_destroy_all(et_dev, is_mgmt);
	et_cqueue_destroy_all(et_dev, is_mgmt);

	// TODO SW-4210: Uncomment when MSIx is enabled
//	et_dev->used_irq_vecs -= vq_common->cq_count;
	wake_up_interruptible_all(&vq_common->waitqueue);
	destroy_workqueue(vq_common->workqueue);

	et_unmap_bar(vq_common->mapped_baseaddr);
}

static inline void interrupt_device(struct et_squeue *sq)
{
	iowrite32(1, sq->vq_common->interrupt_addr);
}

ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count)
{
	struct cmn_header_t *header = buf;
	ssize_t rv;

	if (count < sizeof(*header)) {
		pr_err("VQ[%d]: size too small: %ld", sq->index, count);
		return -EINVAL;
	}

	if (header->size > count) {
		pr_err("VQ[%d]: header contains invalid cmd size",
		       sq->index);
		return -EINVAL;
	}

	mutex_lock(&sq->push_mutex);

	//Write message
	if (!et_circbuffer_push(sq->cb, buf, header->size)) {
		clear_bit(sq->index, sq->vq_common->sq_bitmap);
		pr_err("VQ[%d]: full; no room for message\n", sq->index);
		rv = -EAGAIN;
		goto push_mutex_unlock;
	}

	interrupt_device(sq);
	rv = count;

push_mutex_unlock:
	mutex_unlock(&sq->push_mutex);
	return rv;
}

ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev, bool is_mgmt,
				 u16 sq_index, const char __user *ubuf,
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

ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev, bool is_mgmt,
			       u16 cq_index, char __user *ubuf, size_t count)
{
	struct et_cqueue *cq;
	struct et_msg_node *msg = NULL;
	struct cmn_header_t *header = NULL;
	struct et_dma_info *dma_info = NULL;

	if (et_dev->aborting)
		return -EINTR;

	if (is_mgmt)
		cq = et_dev->mgmt.cq_pptr[cq_index];
	else
		cq = et_dev->ops.cq_pptr[cq_index];

	if (!ubuf || !count)
		return -EINVAL;

	msg = dequeue_msg_node(cq);
	if (!msg) {
		clear_bit(cq->index, cq->vq_common->cq_bitmap);
		pr_err("VQ[%d]: empty; no message to pop\n", cq->index);
		return -EAGAIN;
	}

	if (!(msg->msg))
		return -EINVAL;

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		return -ENOMEM;
	}

	header = (struct cmn_header_t *)msg->msg;

	if (!is_mgmt &&
	    (header->msg_id == DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP ||
	    header->msg_id == DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP)) {
		mutex_lock(&et_dev->ops.dma_rbtree_mutex);
		dma_info = et_dma_search_info(&et_dev->ops.dma_rbtree,
					      header->tag_id);
		if (header->msg_id ==
		    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
			if (dma_info && copy_to_user(dma_info->usr_vaddr,
						     dma_info->kern_vaddr,
						     dma_info->size))
				pr_err("failed to copy DMA buffer\n");
		}
		et_dma_delete_info(&et_dev->ops.dma_rbtree, dma_info);
		mutex_unlock(&et_dev->ops.dma_rbtree_mutex);
	}

	if (copy_to_user(ubuf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		return -ENOMEM;
	}

	return msg->msg_size;
}

ssize_t et_cqueue_pop(struct et_cqueue *cq)
{
	struct cmn_header_t header;
	struct et_msg_node *msg_node;
	ssize_t rv;

	mutex_lock(&cq->pop_mutex);

	// Read the message header
	if (!et_circbuffer_peek(cq->cb, (u8 *)&header.size,
				sizeof(header.size),
				offsetof(struct cmn_header_t, size))) {
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	// If the size is invalid, the message buffer is corrupt and the
	// system is in a bad state. This should never happen.
	// TODO: Add some recovery mechanism
	if (!header.size)
		panic("CQ corrupt: invalid size");

	// Message is for user mode. Save it off.
	msg_node = create_msg_node(header.size);
	if (!msg_node) {
		rv = -ENOMEM;
		goto error_unlock_mutex;
	}

	// MMIO msg into node memory
	if (!et_circbuffer_pop(cq->cb, (u8 *)msg_node->msg, header.size)) {
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	mutex_unlock(&cq->pop_mutex);

	// Enqueue msg node to user msg_list of CQ
	enqueue_msg_node(cq, msg_node);

	wake_up_interruptible(&cq->vq_common->waitqueue);

	return header.size;

error_unlock_mutex:
	mutex_unlock(&cq->pop_mutex);

	return rv;
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
	ssize_t rv;

	// Handle all pending messages in the cqueue
	do {
		rv = et_cqueue_pop(cq);
	} while (rv > 0);
}
