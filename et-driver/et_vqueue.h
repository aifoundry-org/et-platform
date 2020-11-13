// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#ifndef __ET_VQUEUE_H
#define __ET_VQUEUE_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/circ_buf.h>
#include "device_api_spec_privileged.h"
#include "et_mbox.h"
#include "et_vqueue_buffer.h"

struct et_vqueue_desc {
	u8 device_ready;
	u8 host_ready;
	u8 queue_count;
	u16 queue_element_count;
	u16 queue_element_size;
	u64 queue_addr;
} __attribute__ ((__packed__));

struct et_vqueue_header {
	u16 length;
	u16 magic;
};

struct et_vqueue_info {
	u8 master_status;
	u8 slave_status;
	u16 sq_head;
	u16 sq_tail;
	u16 cq_head;
	u16 cq_tail;
} __attribute__ ((__packed__));

struct et_vqueue_buf {
	void __iomem *sq_buf;
	void __iomem *cq_buf;
} __attribute__ ((__packed__));

#if 0
// temporarily being used from et_mbox.h till mbox is present
struct et_msg_node {
	struct list_head list;
	u8 *msg;
	u32 msg_size;
}
#endif

#define VQUEUE_FLAG_ABORT	1
#define VQUEUE_SP		1
#define VQUEUE_MM		0

/* Common among VQs of one type e.g All MM VQs
 * will have its shared instance
 */
struct et_vqueue_common {
	u32 sq_bitmap;
	u32 cq_bitmap;
	u64 queue_addr;
	u8 queue_count;
	u16 queue_buf_count;
	u16 queue_buf_size;
	wait_queue_head_t vqueue_wq;
	void __iomem *interrupt_addr;
	struct workqueue_struct *workqueue;
};

struct et_vqueue {
	struct et_vqueue_common *vqueue_common;
	struct et_vqueue_info *vqueue_info;
	struct et_vqueue_buf *vqueue_buf;
	struct list_head msg_list;
	struct mutex msg_list_mutex;	/* serializes access to msg_list */
	u16 available_buf_count;
	struct mutex buf_count_mutex;   /* serializes access to available_buf_count */
	u16 available_threshold;
	struct mutex threshold_mutex;   /* serializes access to available_threshold */
	struct mutex read_mutex;	/* serializes vqueue read */
	struct mutex write_mutex;	/* serializes vqueue write */
	struct mutex sq_bitmap_mutex;	/* serializes access to sq_bitmap */
	struct mutex cq_bitmap_mutex;	/* serializes access to cq_bitmap */
	struct work_struct isr_work;
	volatile u32 flags;
	bool is_ready;
	u8 index;
};

#define ET_VQUEUE_HEADER_SIZE (sizeof(struct et_vqueue_header))
#define ET_VQUEUE_MSG_ID_SIZE (sizeof(uint64_t))

#define ET_VQUEUE_MIN_MSG_SIZE (ET_VQUEUE_HEADER_SIZE + ET_VQUEUE_MSG_ID_SIZE)

#define ET_VQUEUE_MIN_MSG_LEN (ET_VQUEUE_HEADER_SIZE)

struct et_pci_dev;

void et_vqueue_init(struct et_vqueue *vqueue);

void et_vqueue_destroy(struct et_vqueue *vqueue);

bool et_vqueue_ready(struct et_vqueue *vqueue);

void et_vqueue_reset(struct et_vqueue *vqueue);

ssize_t et_vqueue_write(struct et_vqueue *vqueue, void *buff, size_t count);

ssize_t et_vqueue_write_from_user(struct et_vqueue *vqueue,
				  const char __user *buf, size_t count);

ssize_t et_vqueue_read(struct et_vqueue *vqueue, void *buff, size_t count);

bool usr_message_available(struct et_vqueue *vqueue, struct et_msg_node **msg);

ssize_t et_vqueue_read_to_user(struct et_vqueue *vqueue, char __user *buf,
			       size_t count);

void et_vqueue_isr_bottom(struct et_vqueue *vqueue);

#endif
