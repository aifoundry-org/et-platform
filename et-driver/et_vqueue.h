// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#ifndef __ET_VQUEUE_H
#define __ET_VQUEUE_H

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/rbtree.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "et_circbuffer.h"
#include "et_dev_iface_regs.h"
#include "et_device_api.h"

#define ET_MAX_QUEUES 64

struct et_pci_dev;

// clang-format off

struct et_msg_node {
	struct list_head list;
	u8 *msg;
	u32 msg_size;
};

struct et_vq_common {
	u16 sq_count;
	u16 hpsq_count;
	DECLARE_BITMAP(sq_bitmap, ET_MAX_QUEUES);
	struct mutex sq_bitmap_mutex;

	u16 cq_count;
	DECLARE_BITMAP(cq_bitmap, ET_MAX_QUEUES);
	struct mutex cq_bitmap_mutex;

	u8 intrpt_id;
	u8 intrpt_trg_size;
	void __iomem *intrpt_addr;

	struct workqueue_struct *workqueue;
	wait_queue_head_t waitqueue;
	bool aborting;
	spinlock_t abort_lock;		/* serializes access to aborting */
	struct pci_dev *pdev;
	struct et_mapped_region *trace_region;
};

struct et_squeue {
	u16 index;
	bool is_hpsq;
	struct et_circbuffer __iomem *cb_mem;
	struct et_circbuffer cb;	/* local copy */
	bool cb_mismatched;
	struct mutex push_mutex;	/* serializes access to cb */
	atomic_t sq_threshold;
	struct et_vq_common *vq_common;
};

// clang-format on

ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count);
ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev,
				 bool is_mgmt,
				 bool is_hpsq,
				 u16 sq_index,
				 const char __user *ubuf,
				 size_t count);
bool et_squeue_event_available(struct et_squeue *sq);
bool et_squeue_empty(struct et_squeue *sq);

// clang-format off

struct et_cqueue {
	u16 index;
	struct et_circbuffer __iomem *cb_mem;
	struct et_circbuffer cb;	/* local copy */
	bool cb_mismatched;
	struct mutex pop_mutex;		/* serializes access to cb */
	struct et_vq_common *vq_common;
	struct list_head msg_list;
	struct mutex msg_list_mutex;	/* serializes access to msg_list */
	struct work_struct isr_work;
};

// clang-format on

ssize_t et_cqueue_pop(struct et_cqueue *cq, bool sync_for_host);
ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev,
			       bool is_mgmt,
			       u16 cq_index,
			       char __user *ubuf,
			       size_t count);
bool et_cqueue_msg_available(struct et_cqueue *cq);
bool et_cqueue_event_available(struct et_cqueue *cq);
void et_cqueue_isr_bottom(struct et_cqueue *cq);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt);
void et_vqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

struct et_msg_node *et_dequeue_msg_node(struct et_cqueue *cq);
void et_destroy_msg_list(struct et_cqueue *cq);

#endif
