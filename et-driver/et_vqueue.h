/* SPDX-License-Identifier: GPL-2.0 */

/******************************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 ******************************************************************************/

#ifndef __ET_VQUEUE_H
#define __ET_VQUEUE_H

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "et_circbuffer.h"
#include "et_dev_iface_regs.h"
#include "et_device_api.h"
#include "et_sysfs.h"

/* Maximum allowed number of VQs */
#define ET_MAX_QUEUES 64

struct et_pci_dev;

/**
 * struct et_msg_node - CQ message node struct
 * @list: struct list_head node / glue logic for lists
 * @msg: Pointer to msg buffer
 * @msg_size: Size of msg buffer in bytes
 */
struct et_msg_node {
	struct list_head list;
	u8 *msg;
	u32 msg_size;
};

/**
 * struct et_vq_common - Common information of all VQs
 * @sq_count: Number of SQs
 * @sq_size: Size of a SQ in bytes
 * @hp_sq_count: Number of HPSQs
 * @hp_sq_size: Size of an HPSQ in bytes
 * @sq_bitmap: SQ availability bitmap (set bit indicates free space in SQ)
 * @sq_workqueue: Workqueue to process SQ work items
 * @cq_count: Number of SQs
 * @cq_cize: Size of a SQ in bytes
 * @cq_bitmap: CQ availability bitmap (set bit indicates filled space in CQ)
 * @cq_workqueue: Workqueue to process CQ work items
 * @intrpt_id: MBox interrupt ID
 * @intrpt_trg_size: Size of interrupt trigger register in bits
 * @intrpt_addr: IOMEM address that triggers MBox interrupt when written
 * @waitqueue: A waitqueue struct to synchronize wait on SQ/CQ bitmaps with
 *	       EPOLL thread
 * @pdev: Pointer to struct pci_dev
 * @trace_region: Pointer to a BAR mapped region of SP traces
 */
struct et_vq_common {
	u16 sq_count;
	u16 sq_size;
	u16 hp_sq_count;
	u16 hp_sq_size;
	DECLARE_BITMAP(sq_bitmap, ET_MAX_QUEUES);
	/**
	 * @sq_bitmap_mutex: Serializes access to sq_bitmap
	 */
	struct mutex sq_bitmap_mutex;
	struct workqueue_struct *sq_workqueue;

	u16 cq_count;
	u16 cq_size;
	DECLARE_BITMAP(cq_bitmap, ET_MAX_QUEUES);
	/**
	 * @cq_bitmap_mutex: Serializes access to cq_bitmap
	 */
	struct mutex cq_bitmap_mutex;
	struct workqueue_struct *cq_workqueue;

	u8 intrpt_id;
	u8 intrpt_trg_size;
	void __iomem *intrpt_addr;

	wait_queue_head_t waitqueue;
	struct pci_dev *pdev;
	struct et_mapped_region *trace_region;
};

/**
 * struct et_squeue - Submission Queue information
 * @index: Index of SQ
 * @is_hq_sq: HPSQ if set otherwise normal SQ
 * @cb_mem: IOMEM pointer of circular buffer memory
 * @cb: Local copy of circular buffer
 * @cb_mismatched: Tracks mismatch between cb and cb_mem
 * @sq_threshold: SQ threshold in bytes
 * @vq_common: Pointer to struct vq_common
 * @isr_work: Work ISR for this SQ
 * @stats: SQ statistics for SysFS
 */
struct et_squeue {
	u16 index;
	bool is_hp_sq;
	struct et_circbuffer __iomem *cb_mem;
	struct et_circbuffer cb;
	bool cb_mismatched;
	/**
	 * @push_mutex: serializes access to cb
	 */
	struct mutex push_mutex;
	atomic_t sq_threshold;
	struct et_vq_common *vq_common;
	struct work_struct isr_work;
	struct et_vq_stats stats;
};

/**
 * struct et_cqueue - Completion Queue information
 * @index: Index of CQ
 * @cb_mem: IOMEM pointer of circular buffer memory
 * @cb: Local copy of circular buffer
 * @cb_mismatched: Tracks mismatch between cb and cb_mem
 * @vq_common: Pointer to struct vq_common
 * @isr_work: Work ISR for this CQ
 * @msg_list: list of pointer of message nodes
 * @stats: CQ statistics for SysFS
 */
struct et_cqueue {
	u16 index;
	struct et_circbuffer __iomem *cb_mem;
	struct et_circbuffer cb;
	bool cb_mismatched;
	/**
	 * @pop_mutex: serializes access to cb
	 */
	struct mutex pop_mutex;
	struct et_vq_common *vq_common;
	struct work_struct isr_work;
	struct list_head msg_list;
	/**
	 * @msg_list_mutex: serializes access to msg_list
	 */
	struct mutex msg_list_mutex;
	struct et_vq_stats stats;
};

/**
 * struct et_vq_data - Packed information of complete VQs
 * @vq_common: Common information to all VQs
 * @sqs: Array of SQs
 * @hp_sqs: Array of HPSQs
 * @cqs: Array of CQs
 */
struct et_vq_data {
	struct et_vq_common vq_common;
	struct et_squeue *sqs;
	struct et_squeue *hp_sqs;
	struct et_cqueue *cqs;
};

ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev, bool is_mgmt,
				 bool is_hp_sq, u16 sq_index,
				 const char __user *ubuf, size_t count);
ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count);
void et_squeue_sync_cb_for_host(struct et_squeue *sq);
void et_squeue_sync_bitmap(struct et_squeue *sq);
bool et_squeue_empty(struct et_squeue *sq);

ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev, bool is_mgmt,
			       u16 cq_index, char __user *ubuf, size_t count);
ssize_t et_cqueue_pop(struct et_cqueue *cq, bool sync_for_host);
void et_cqueue_sync_cb_for_host(struct et_cqueue *cq);
bool et_cqueue_msg_available(struct et_cqueue *cq);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt);
void et_vqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

#endif
