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

static void mm_reset_completion_callback(struct et_cqueue *cq,
					 struct device_mgmt_rsp_hdr_t *rsp)
{
	int rv = 0;
	struct et_pci_dev *et_dev = pci_get_drvdata(cq->vq_common->pdev);

	if (rsp->status != DEV_OPS_API_MM_RESET_RESPONSE_COMPLETE) {
		dev_err(&et_dev->pdev->dev,
			"MM reset failed!, status: %d\n",
			rsp->status);
		return;
	}

	mutex_lock(&et_dev->ops.reset_mutex);

	if (!et_dev->ops.is_resetting) {
		dev_err(&et_dev->pdev->dev,
			"MM reset response received but no reset was triggered!");
		goto unlock_reset_mutex;
	}
	et_ops_dev_destroy(et_dev, false);
	rv = et_ops_dev_init(et_dev, 0, false);
	if (rv)
		dev_err(&et_dev->pdev->dev,
			"Ops device re-initialization failed, errno: %d\n",
			-rv);

	et_dev->ops.is_resetting = false;

unlock_reset_mutex:
	mutex_unlock(&et_dev->ops.reset_mutex);
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

static irqreturn_t et_pcie_sq_isr(int irq, void *sq_id)
{
	int i;
	struct et_squeue *sqs = (struct et_squeue *)sq_id;
	struct et_vq_common *vq_common = sqs[0].vq_common;

	for (i = 0; i < vq_common->sq_count; i++)
		queue_work(vq_common->sq_workqueue, &sqs[i].isr_work);

	return IRQ_HANDLED;
}

void et_squeue_sync_cb_for_host(struct et_squeue *sq);

static void et_sq_isr_work(struct work_struct *work)
{
	struct et_squeue *sq = container_of(work, struct et_squeue, isr_work);

	// Check for SQ availability and wake up the waitqueue if available
	if (test_bit(sq->index, sq->vq_common->sq_bitmap))
		goto update_sq_bitmap;

	et_squeue_sync_cb_for_host(sq);

update_sq_bitmap:
	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold)) {
		set_bit(sq->index, sq->vq_common->sq_bitmap);
		wake_up_interruptible(&sq->vq_common->waitqueue);
	}

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);
}

static irqreturn_t et_pcie_cq_isr(int irq, void *cq_id)
{
	int i;
	struct et_cqueue *cqs = (struct et_cqueue *)cq_id;
	struct et_vq_common *vq_common = cqs[0].vq_common;

	for (i = 0; i < vq_common->cq_count; i++)
		queue_work(vq_common->cq_workqueue, &cqs[i].isr_work);

	return IRQ_HANDLED;
}

static void et_cq_isr_work(struct work_struct *work)
{
	struct et_cqueue *cq = container_of(work, struct et_cqueue, isr_work);

	et_cqueue_isr_bottom(cq);
}

static ssize_t et_high_priority_squeue_init_all(struct et_pci_dev *et_dev,
						bool is_mgmt)
{
	ssize_t i;
	struct et_mapped_region *vq_region;
	u8 __iomem *hp_sq_baseaddr;
	struct et_vq_data *vq_data;
	u16 hp_sq_size;

	if (is_mgmt) {
		dev_dbg(&et_dev->pdev->dev, "Mgmt: HP SQs are not supported\n");
		return 0;
	}

	if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid) {
		return -EINVAL;
	}

	vq_data = &et_dev->ops.vq_data;
	vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	hp_sq_baseaddr = (u8 __iomem *)vq_region->io.mapped_baseaddr +
			 et_dev->ops.dir_vq.hp_sq_offset;
	hp_sq_size = et_dev->ops.dir_vq.hp_sq_size;

	vq_data->hp_sqs = kmalloc_array(vq_data->vq_common.hp_sq_count,
					sizeof(*vq_data->hp_sqs),
					GFP_KERNEL);
	if (!vq_data->hp_sqs)
		return -ENOMEM;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		vq_data->hp_sqs[i].index = i;
		vq_data->hp_sqs[i].is_hp_sq = true;
		vq_data->hp_sqs[i].vq_common = &vq_data->vq_common;
		vq_data->hp_sqs[i].cb_mem =
			(struct et_circbuffer __iomem *)hp_sq_baseaddr;
		et_ioread(vq_data->hp_sqs[i].cb_mem,
			  0,
			  (u8 *)&vq_data->hp_sqs[i].cb,
			  sizeof(vq_data->hp_sqs[i].cb));
		vq_data->hp_sqs[i].cb_mismatched = false;
		hp_sq_baseaddr += hp_sq_size;

		mutex_init(&vq_data->hp_sqs[i].push_mutex);
		et_vq_stats_init(&vq_data->hp_sqs[i].stats);
	}

	return 0;
}

static ssize_t et_squeue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i, rv;
	unsigned long vec_idx;
	struct et_mapped_region *vq_region;
	struct et_vq_data *vq_data;
	u8 __iomem *sq_baseaddr;
	u16 sq_size;

	if (is_mgmt) {
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->mgmt.vq_data;
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
		sq_baseaddr = (u8 __iomem *)vq_region->io.mapped_baseaddr +
			      et_dev->mgmt.dir_vq.sq_offset;
		sq_size = et_dev->mgmt.dir_vq.sq_size;

		vec_idx = ET_MGMT_SQ_VEC_IDX;
	} else {
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->ops.vq_data;
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
		sq_baseaddr = (u8 __iomem *)vq_region->io.mapped_baseaddr +
			      et_dev->ops.dir_vq.sq_offset;
		sq_size = et_dev->ops.dir_vq.sq_size;

		vec_idx = ET_OPS_SQ_VEC_IDX;
	}

	// Initialize sq_workqueue
	vq_data->vq_common.sq_workqueue =
		alloc_workqueue("%s:%s%d_sqwq",
				WQ_MEM_RECLAIM | WQ_UNBOUND,
				vq_data->vq_common.sq_count,
				dev_name(&et_dev->pdev->dev),
				(is_mgmt) ? "mgmt" : "ops",
				et_dev->devnum);
	if (!vq_data->vq_common.sq_workqueue)
		return -ENOMEM;

	vq_data->sqs = kmalloc_array(vq_data->vq_common.sq_count,
				     sizeof(*vq_data->sqs),
				     GFP_KERNEL);
	if (!vq_data->sqs) {
		rv = -ENOMEM;
		goto error_destroy_sq_workqueue;
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		vq_data->sqs[i].index = i;
		vq_data->sqs[i].is_hp_sq = false;
		vq_data->sqs[i].vq_common = &vq_data->vq_common;
		vq_data->sqs[i].cb_mem =
			(struct et_circbuffer __iomem *)sq_baseaddr;
		et_ioread(vq_data->sqs[i].cb_mem,
			  0,
			  (u8 *)&vq_data->sqs[i].cb,
			  sizeof(vq_data->sqs[i].cb));
		vq_data->sqs[i].cb_mismatched = false;
		sq_baseaddr += sq_size;

		mutex_init(&vq_data->sqs[i].push_mutex);
		atomic_set(&vq_data->sqs[i].sq_threshold,
			   (sq_size - sizeof(struct et_circbuffer)) / 4);

		// Init statistics before work handler
		et_vq_stats_init(&vq_data->sqs[i].stats);

		INIT_WORK(&vq_data->sqs[i].isr_work, et_sq_isr_work);
		queue_work(vq_data->vq_common.sq_workqueue,
			   &vq_data->sqs[i].isr_work);
		flush_workqueue(vq_data->vq_common.sq_workqueue);
	}

	rv = request_irq(pci_irq_vector(et_dev->pdev, vec_idx),
			 et_pcie_sq_isr,
			 0,
			 devm_kasprintf(&et_dev->pdev->dev,
					GFP_KERNEL,
					"%s%d_irq%ld",
					is_mgmt ? "mgmt" : "ops",
					et_dev->devnum,
					vec_idx),
			 (void *)vq_data->sqs);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "request irq failed\n");
		goto error_free_sqs;
	}

	return rv;

error_free_sqs:
	kfree(vq_data->sqs);

error_destroy_sq_workqueue:
	destroy_workqueue(vq_data->vq_common.sq_workqueue);

	return rv;
}

static ssize_t et_cqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i, rv;
	unsigned long vec_idx;
	struct et_mapped_region *vq_region;
	struct et_vq_data *vq_data;
	u8 __iomem *cq_baseaddr;
	u16 cq_size;

	if (is_mgmt) {
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->mgmt.vq_data;
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
		cq_baseaddr = (u8 __iomem *)vq_region->io.mapped_baseaddr +
			      et_dev->mgmt.dir_vq.cq_offset;
		cq_size = et_dev->mgmt.dir_vq.cq_size;

		vec_idx = ET_MGMT_CQ_VEC_IDX;
	} else {
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->ops.vq_data;
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
		cq_baseaddr = (u8 __iomem *)vq_region->io.mapped_baseaddr +
			      et_dev->ops.dir_vq.cq_offset;
		cq_size = et_dev->ops.dir_vq.cq_size;

		vec_idx = ET_OPS_CQ_VEC_IDX;
	}

	// Initialize cq_workqueue
	vq_data->vq_common.cq_workqueue =
		alloc_workqueue("%s:%s%d_cqwq",
				WQ_MEM_RECLAIM | WQ_UNBOUND,
				vq_data->vq_common.cq_count,
				dev_name(&et_dev->pdev->dev),
				(is_mgmt) ? "mgmt" : "ops",
				et_dev->devnum);
	if (!vq_data->vq_common.cq_workqueue)
		return -ENOMEM;

	vq_data->cqs = kmalloc_array(vq_data->vq_common.cq_count,
				     sizeof(*vq_data->cqs),
				     GFP_KERNEL);
	if (!vq_data->cqs) {
		rv = -ENOMEM;
		goto error_destroy_cq_workqueue;
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		vq_data->cqs[i].index = i;
		vq_data->cqs[i].vq_common = &vq_data->vq_common;
		vq_data->cqs[i].cb_mem =
			(struct et_circbuffer __iomem *)cq_baseaddr;
		et_ioread(vq_data->cqs[i].cb_mem,
			  0,
			  (u8 *)&vq_data->cqs[i].cb,
			  sizeof(vq_data->cqs[i].cb));
		vq_data->cqs[i].cb_mismatched = false;
		cq_baseaddr += cq_size;

		mutex_init(&vq_data->cqs[i].pop_mutex);
		INIT_LIST_HEAD(&vq_data->cqs[i].msg_list);
		mutex_init(&vq_data->cqs[i].msg_list_mutex);

		// Init statistics before work handler
		et_vq_stats_init(&vq_data->cqs[i].stats);

		INIT_WORK(&vq_data->cqs[i].isr_work, et_cq_isr_work);
		queue_work(vq_data->vq_common.cq_workqueue,
			   &vq_data->cqs[i].isr_work);
		flush_workqueue(vq_data->vq_common.cq_workqueue);
	}

	rv = request_irq(pci_irq_vector(et_dev->pdev, vec_idx),
			 et_pcie_cq_isr,
			 0,
			 devm_kasprintf(&et_dev->pdev->dev,
					GFP_KERNEL,
					"%s%d_irq%ld",
					is_mgmt ? "mgmt" : "ops",
					et_dev->devnum,
					vec_idx),
			 (void *)vq_data->cqs);
	if (rv) {
		dev_err(&et_dev->pdev->dev, "request irq failed\n");
		goto error_free_cqs;
	}

	return 0;

error_free_cqs:
	kfree(vq_data->cqs);

error_destroy_cq_workqueue:
	destroy_workqueue(vq_data->vq_common.cq_workqueue);

	return rv;
}

static void et_high_priority_squeue_destroy_all(struct et_pci_dev *et_dev,
						bool is_mgmt);
static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t rv;
	struct et_vq_common *vq_common;
	struct et_mapped_region *intrpt_region =
		&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_INTRPT_TRG];
	int vq_stats_gid;

	if (!intrpt_region->is_valid)
		return -EINVAL;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_data.vq_common;
		vq_common->sq_count = et_dev->mgmt.dir_vq.sq_count;
		vq_common->sq_size = et_dev->mgmt.dir_vq.sq_size;
		vq_common->hp_sq_count = 0;
		vq_common->hp_sq_size = 0;
		vq_common->cq_count = et_dev->mgmt.dir_vq.cq_count;
		vq_common->cq_size = et_dev->mgmt.dir_vq.cq_size;
		vq_common->intrpt_id = et_dev->mgmt.dir_vq.intrpt_id;
		vq_common->intrpt_trg_size =
			et_dev->mgmt.dir_vq.intrpt_trg_size;
		vq_common->intrpt_addr =
			(u8 __iomem *)intrpt_region->io.mapped_baseaddr +
			et_dev->mgmt.dir_vq.intrpt_trg_offset;

		// Save Mgmt trace region reference
		vq_common->trace_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_SPFW_TRACE];

		vq_stats_gid = ET_SYSFS_GID_MGMT_VQ_STATS;
	} else {
		vq_common = &et_dev->ops.vq_data.vq_common;
		vq_common->sq_count = et_dev->ops.dir_vq.sq_count;
		vq_common->sq_size = et_dev->ops.dir_vq.sq_size;
		vq_common->hp_sq_count = et_dev->ops.dir_vq.hp_sq_count;
		vq_common->hp_sq_size = et_dev->ops.dir_vq.hp_sq_size;
		vq_common->cq_count = et_dev->ops.dir_vq.cq_count;
		vq_common->cq_size = et_dev->ops.dir_vq.cq_size;
		vq_common->intrpt_id = et_dev->ops.dir_vq.intrpt_id;
		vq_common->intrpt_trg_size = et_dev->ops.dir_vq.intrpt_trg_size;
		vq_common->intrpt_addr =
			(u8 __iomem *)intrpt_region->io.mapped_baseaddr +
			et_dev->ops.dir_vq.intrpt_trg_offset;

		vq_stats_gid = ET_SYSFS_GID_OPS_VQ_STATS;
	}

	bitmap_zero(vq_common->sq_bitmap, ET_MAX_QUEUES);
	mutex_init(&vq_common->sq_bitmap_mutex);
	bitmap_zero(vq_common->cq_bitmap, ET_MAX_QUEUES);
	mutex_init(&vq_common->cq_bitmap_mutex);
	init_waitqueue_head(&vq_common->waitqueue);
	vq_common->pdev = et_dev->pdev;

	rv = et_sysfs_add_group(et_dev, vq_stats_gid);
	if (rv)
		return rv;

	rv = et_high_priority_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_sysfs_remove_group;

	rv = et_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_high_priority_squeue_destroy_all;

	rv = et_cqueue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_squeue_destroy_all;

	return rv;

error_squeue_destroy_all:
	et_squeue_destroy_all(et_dev, is_mgmt);

error_high_priority_squeue_destroy_all:
	et_high_priority_squeue_destroy_all(et_dev, is_mgmt);

error_sysfs_remove_group:
	et_sysfs_remove_group(et_dev, vq_stats_gid);

	return rv;
}

static void et_high_priority_squeue_destroy_all(struct et_pci_dev *et_dev,
						bool is_mgmt)
{
	ssize_t i;
	struct et_vq_data *vq_data;

	if (is_mgmt)
		return;

	vq_data = &et_dev->ops.vq_data;
	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		mutex_destroy(&vq_data->hp_sqs[i].push_mutex);
		vq_data->hp_sqs[i].cb_mem = NULL;
		vq_data->hp_sqs[i].vq_common = NULL;
	}

	kfree(vq_data->hp_sqs);
}

static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	int i;
	unsigned long vec_idx;
	struct et_vq_data *vq_data;

	if (is_mgmt) {
		vec_idx = ET_MGMT_SQ_VEC_IDX;
		vq_data = &et_dev->mgmt.vq_data;
	} else {
		vec_idx = ET_OPS_SQ_VEC_IDX;
		vq_data = &et_dev->ops.vq_data;
	}

	free_irq(pci_irq_vector(et_dev->pdev, vec_idx), (void *)vq_data->sqs);

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		cancel_work_sync(&vq_data->sqs[i].isr_work);
		mutex_destroy(&vq_data->sqs[i].push_mutex);
		vq_data->sqs[i].cb_mem = NULL;
		vq_data->sqs[i].vq_common = NULL;
	}

	kfree(vq_data->sqs);
	destroy_workqueue(vq_data->vq_common.sq_workqueue);
}

static void et_cqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	int i;
	unsigned long vec_idx;
	struct et_vq_data *vq_data;

	if (is_mgmt) {
		vec_idx = ET_MGMT_CQ_VEC_IDX;
		vq_data = &et_dev->mgmt.vq_data;
	} else {
		vec_idx = ET_OPS_CQ_VEC_IDX;
		vq_data = &et_dev->ops.vq_data;
	}

	free_irq(pci_irq_vector(et_dev->pdev, vec_idx), (void *)vq_data->cqs);

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		cancel_work_sync(&vq_data->cqs[i].isr_work);
		mutex_destroy(&vq_data->cqs[i].pop_mutex);
		et_destroy_msg_list(&vq_data->cqs[i]);
		mutex_destroy(&vq_data->cqs[i].msg_list_mutex);
		vq_data->cqs[i].cb_mem = NULL;
		vq_data->cqs[i].vq_common = NULL;
	}

	kfree(vq_data->cqs);
	destroy_workqueue(vq_data->vq_common.cq_workqueue);
}

void et_vqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_vq_data *vq_data;
	int vq_stats_gid;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	vq_stats_gid = is_mgmt ? ET_SYSFS_GID_MGMT_VQ_STATS :
				 ET_SYSFS_GID_OPS_VQ_STATS;
	// Memory barrier to ensure changes for the sleeper on waitqueue
	// before it awakes
	smp_mb();
	wake_up_all(&vq_data->vq_common.waitqueue);
	while (waitqueue_active(&vq_data->vq_common.waitqueue))
		msleep(1);

	et_cqueue_destroy_all(et_dev, is_mgmt);
	et_squeue_destroy_all(et_dev, is_mgmt);
	et_high_priority_squeue_destroy_all(et_dev, is_mgmt);

	et_sysfs_remove_group(et_dev, vq_stats_gid);

	mutex_destroy(&vq_data->vq_common.sq_bitmap_mutex);
	mutex_destroy(&vq_data->vq_common.cq_bitmap_mutex);
}

static inline void interrupt_device(struct et_squeue *sq)
{
	switch (sq->vq_common->intrpt_trg_size) {
	case 1:
		iowrite8(sq->vq_common->intrpt_id, sq->vq_common->intrpt_addr);
		break;
	case 2:
		iowrite16(sq->vq_common->intrpt_id, sq->vq_common->intrpt_addr);
		break;
	case 4:
		iowrite32(sq->vq_common->intrpt_id, sq->vq_common->intrpt_addr);
		break;
	case 8:
		iowrite64(sq->vq_common->intrpt_id, sq->vq_common->intrpt_addr);
	}
}

ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count)
{
	struct cmn_header_t *header = buf;
	ssize_t rv = count;

	if (count < sizeof(*header)) {
		pr_err("SQ[%d]: size too small: %ld", sq->index, count);
		return -EINVAL;
	}

	if (header->size > count) {
		pr_err("SQ[%d]: header contains invalid cmd size", sq->index);
		return -EINVAL;
	}

	if (sq->cb_mismatched) {
		pr_err("SQ[%d] corrupt: circbuffer header invalid!", sq->index);
		return -ENOTRECOVERABLE;
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

	atomic64_inc(&sq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
	et_rate_entry_update(1, &sq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
	atomic64_add(header->size,
		     &sq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
	et_rate_entry_update(header->size,
			     &sq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

update_sq_bitmap:
	mutex_unlock(&sq->push_mutex);

	if (sq->is_hp_sq)
		return rv;

	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) < atomic_read(&sq->sq_threshold)) {
		clear_bit(sq->index, sq->vq_common->sq_bitmap);
		wake_up_interruptible(&sq->vq_common->waitqueue);
	}

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);

	return rv;
}

ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev,
				 bool is_mgmt,
				 bool is_hp_sq,
				 u16 sq_index,
				 const char __user *ubuf,
				 size_t count)
{
	struct et_vq_data *vq_data;
	struct et_squeue *sq;
	u8 *kern_buf;
	ssize_t rv;

	vq_data = (is_mgmt) ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	sq = (is_hp_sq) ? &vq_data->hp_sqs[sq_index] : &vq_data->sqs[sq_index];

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
		rv = -EFAULT;
		goto free_kern_buf;
	}

	rv = et_squeue_push(sq, kern_buf, count);

free_kern_buf:
	kfree(kern_buf);

	return rv;
}

void et_squeue_sync_cb_for_host(struct et_squeue *sq)
{
	u64 head_local;

	mutex_lock(&sq->push_mutex);
	head_local = sq->cb.head;
	et_ioread(sq->cb_mem, 0, (u8 *)&sq->cb, sizeof(sq->cb));

	if (head_local != sq->cb.head) {
		pr_err("SQ[%d] sync: head mismatched, head_local: %lld, head_remote: %lld",
		       sq->index,
		       head_local,
		       sq->cb.head);
		sq->cb_mismatched = true;
	}

	mutex_unlock(&sq->push_mutex);
}

void et_squeue_sync_bitmap(struct et_squeue *sq)
{
	et_squeue_sync_cb_for_host(sq);

	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold))
		set_bit(sq->index, sq->vq_common->sq_bitmap);
	else
		clear_bit(sq->index, sq->vq_common->sq_bitmap);
	wake_up_interruptible(&sq->vq_common->waitqueue);

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);
}

bool et_squeue_empty(struct et_squeue *sq)
{
	if (!sq)
		return false;

	et_squeue_sync_cb_for_host(sq);

	if (et_circbuffer_used(&sq->cb) != 0)
		return false;

	return true;
}

ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev,
			       bool is_mgmt,
			       u16 cq_index,
			       char __user *ubuf,
			       size_t count)
{
	struct et_vq_data *vq_data;
	struct et_cqueue *cq;
	struct et_msg_node *msg = NULL;
	ssize_t rv;

	if (!ubuf || !count)
		return -EINVAL;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	cq = &vq_data->cqs[cq_index];
	if (cq->cb_mismatched) {
		pr_err("CQ[%d] corrupt: circbuffer header invalid!", cq->index);
		return -ENOTRECOVERABLE;
	}

	msg = et_dequeue_msg_node(cq);
	if (!msg || !(msg->msg)) {
		// Empty; no message to POP, returning EAGAIN
		rv = -EAGAIN;
		goto update_cq_bitmap;
	}

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		// Enqueue the msg again so the userspace can retry with a
		// larger buffer
		enqueue_msg_node(cq, msg);
		return -EINVAL;
	}

	if (copy_to_user(ubuf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		rv = -EFAULT;
		goto free_msg_node;
	}

	rv = msg->msg_size;

free_msg_node:
	destroy_msg_node(msg);

update_cq_bitmap:
	// Update cq_bitmap
	mutex_lock(&cq->vq_common->cq_bitmap_mutex);

	if (!et_cqueue_msg_available(cq)) {
		clear_bit(cq->index, cq->vq_common->cq_bitmap);
		wake_up_interruptible(&cq->vq_common->waitqueue);
	}

	mutex_unlock(&cq->vq_common->cq_bitmap_mutex);

	return rv;
}

ssize_t et_cqueue_pop(struct et_cqueue *cq, bool sync_for_host)
{
	struct cmn_header_t header;
	struct et_msg_node *msg_node;
	struct device_mgmt_event_msg_t mgmt_event;
	ssize_t rv;

	if (cq->cb_mismatched) {
		pr_err("et_cqueue_pop: CQ[%d]: circbuffer header invalid!",
		       cq->index);
		return -ENOTRECOVERABLE;
	}

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
	if (!header.size) {
		pr_err("et_cqueue_pop: CQ[%d]: invalid size!", cq->index);
		rv = -ENOTRECOVERABLE;
		goto error_unlock_mutex;
	}

	// Check if this is a mgmt event, handle accordingly
	if (header.msg_id >= DEV_MGMT_API_MID_EVENTS_BEGIN &&
	    header.msg_id <= DEV_MGMT_API_MID_EVENTS_END) {
		memcpy((u8 *)&mgmt_event.event_info,
		       (u8 *)&header,
		       sizeof(header));

		if (!et_circbuffer_pop(&cq->cb,
				       cq->cb_mem,
				       (u8 *)&mgmt_event + sizeof(header),
				       header.size - sizeof(header),
				       ET_CB_SYNC_FOR_DEVICE)) {
			rv = -EAGAIN;
			goto error_unlock_mutex;
		}
		mutex_unlock(&cq->pop_mutex);

		rv = et_handle_device_event(cq, &mgmt_event);

		atomic64_inc(
			&cq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
		et_rate_entry_update(
			1,
			&cq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
		atomic64_add(
			header.size + sizeof(header),
			&cq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
		et_rate_entry_update(
			header.size + sizeof(header),
			&cq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

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
		destroy_msg_node(msg_node);
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	mutex_unlock(&cq->pop_mutex);

	atomic64_inc(&cq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
	et_rate_entry_update(1, &cq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
	atomic64_add(header.size + sizeof(header),
		     &cq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
	et_rate_entry_update(header.size + sizeof(header),
			     &cq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

	// Check for MM reset command and complete post reset steps
	if (header.msg_id == DEV_MGMT_API_MID_MM_RESET)
		mm_reset_completion_callback(
			cq,
			(struct device_mgmt_rsp_hdr_t *)msg_node->msg);

	// Enqueue msg node to user msg_list of CQ
	enqueue_msg_node(cq, msg_node);

	mutex_lock(&cq->vq_common->cq_bitmap_mutex);
	set_bit(cq->index, cq->vq_common->cq_bitmap);
	mutex_unlock(&cq->vq_common->cq_bitmap_mutex);

	wake_up_interruptible(&cq->vq_common->waitqueue);

	return header.size;

error_unlock_mutex:
	mutex_unlock(&cq->pop_mutex);

	return rv;
}

void et_cqueue_sync_cb_for_host(struct et_cqueue *cq)
{
	u64 tail_local;

	mutex_lock(&cq->pop_mutex);
	tail_local = cq->cb.tail;
	et_ioread(cq->cb_mem, 0, (u8 *)&cq->cb, sizeof(cq->cb));

	if (tail_local != cq->cb.tail) {
		pr_err("CQ[%d] sync: tail mismatched, tail_local: %lld, tail_remote: %lld",
		       cq->index,
		       tail_local,
		       cq->cb.tail);
		cq->cb_mismatched = true;
	}

	mutex_unlock(&cq->pop_mutex);
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
