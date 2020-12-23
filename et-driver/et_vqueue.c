// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include "et_vqueue.h"
#include "et_dma.h"
#include "et_io.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "et_pci_dev.h"

enum et_vqueue_status {
	VQUEUE_STATUS_NOT_READY = 0U,
	VQUEUE_STATUS_READY = 1U,
	VQUEUE_STATUS_WAITING = 2U,
	VQUEUE_STATUS_ERROR = 3U
} __attribute__ ((__packed__));

#define ET_VQUEUE_MAGIC 0xBEEF

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

static void destroy_msg_node(struct et_msg_node *node)
{
	kfree(node->msg);
	kfree(node);
}

static void destroy_msg_list(struct et_vqueue *vqueue)
{
	struct list_head *pos, *next;
	struct et_msg_node *node;

	list_for_each_safe(pos, next, &vqueue->msg_list) {
		node = list_entry(pos, struct et_msg_node, list);
		list_del(pos);
		destroy_msg_node(node);
	}
}

void et_vqueue_init(struct et_vqueue *vqueue)
{
	vqueue->flags = 0;

	mutex_init(&vqueue->msg_list_mutex);
	mutex_init(&vqueue->read_mutex);
	mutex_init(&vqueue->write_mutex);
	mutex_init(&vqueue->buf_count_mutex);
	mutex_init(&vqueue->threshold_mutex);
	mutex_init(&vqueue->cq_bitmap_mutex);
	mutex_init(&vqueue->sq_bitmap_mutex);

	INIT_LIST_HEAD(&vqueue->msg_list);

	//The host is always the VQ slave; the SoC inits other fields
	iowrite8(VQUEUE_STATUS_NOT_READY, &vqueue->vqueue_info->slave_status);

	et_vqueue_reset(vqueue);
}

void et_vqueue_destroy(struct et_vqueue *vqueue)
{
	if (vqueue->vqueue_info)
		iowrite8(VQUEUE_STATUS_NOT_READY,
			 &vqueue->vqueue_info->slave_status);

	mutex_lock(&vqueue->msg_list_mutex);
	vqueue->flags = VQUEUE_FLAG_ABORT;
	mutex_unlock(&vqueue->msg_list_mutex);

	mutex_destroy(&vqueue->sq_bitmap_mutex);
	mutex_destroy(&vqueue->cq_bitmap_mutex);
	mutex_destroy(&vqueue->threshold_mutex);
	mutex_destroy(&vqueue->buf_count_mutex);
	mutex_destroy(&vqueue->write_mutex);
	mutex_destroy(&vqueue->read_mutex);
	mutex_destroy(&vqueue->msg_list_mutex);

	destroy_msg_list(vqueue);
	cancel_work_sync(&vqueue->isr_work);

	vqueue->vqueue_info = NULL;
	vqueue->is_ready = false;
}

bool et_vqueue_ready(struct et_vqueue *vqueue)
{
	return vqueue->is_ready;
}

static void interrupt_vqueue(void __iomem *interrupt_addr)
{
	iowrite32(1, interrupt_addr);
}

void et_vqueue_reset(struct et_vqueue *vqueue)
{
	u8 slave_status;

	mutex_lock(&vqueue->msg_list_mutex);
	mutex_lock(&vqueue->read_mutex);
	mutex_lock(&vqueue->write_mutex);

	destroy_msg_list(vqueue);

	//Request that the SoC reset the VQ
	iowrite8(VQUEUE_STATUS_WAITING, &vqueue->vqueue_info->slave_status);

	//Flush PCIe writes: this makes sure the status changes are visible
	//to the interrupt routines in the MM and SP
	slave_status = ioread8(&vqueue->vqueue_info->slave_status);

	//Notify SoC status changed
	interrupt_vqueue(vqueue->vqueue_common->interrupt_addr);

	//Flush PCIe writes again for complete safety: the interrupt_vqueue()
	//routine is really just an iowrite32(), flush them as well
	slave_status = ioread8(&vqueue->vqueue_info->slave_status);

	mutex_unlock(&vqueue->write_mutex);
	mutex_unlock(&vqueue->read_mutex);
	mutex_unlock(&vqueue->msg_list_mutex);

	vqueue->is_ready = false;
	vqueue->available_buf_count = vqueue->vqueue_common->queue_buf_count;
	vqueue->available_threshold = 1;
	vqueue->vqueue_common->sq_bitmap &= ~((u32)1 << vqueue->index);
	vqueue->vqueue_common->cq_bitmap &= ~((u32)1 << vqueue->index);
}

static void et_vqueue_handshaking(struct et_vqueue *vqueue);

ssize_t et_vqueue_write(struct et_vqueue *vqueue, void *buf, size_t count)
{
	volatile u16 head_index, tail_index;
	u16 free_buffers;
	void __iomem *sq_buf;
	ssize_t offset, rc;
	const struct et_vqueue_header header = { .length = (u16)count,
					       .magic = ET_VQUEUE_MAGIC };
	if (!et_vqueue_ready(vqueue)) {
		et_vqueue_handshaking(vqueue);
		return -EAGAIN;
	}

	if (count < 1 || count > U16_MAX)
		return -EINVAL;

	mutex_lock(&vqueue->write_mutex);

	head_index = ioread16(&vqueue->vqueue_info->sq_head);
	tail_index = ioread16(&vqueue->vqueue_info->sq_tail);

	free_buffers = CIRC_SPACE(head_index, tail_index,
				  vqueue->vqueue_common->queue_buf_count);

	if (free_buffers < 1) {
		pr_err("VQ[%d]: full; no room for message\n", vqueue->index);
		rc = -ENOMEM;
		goto write_mutex_unlock;
	}

	sq_buf = vqueue->vqueue_buf->sq_buf +
			(head_index * vqueue->vqueue_common->queue_buf_size);

	//Write header
	offset = 0;
	offset = et_vqueue_buffer_write(sq_buf, offset,
					vqueue->vqueue_common->queue_buf_size,
					(u8 *)&header, ET_VQUEUE_HEADER_SIZE);
	if (offset < 0) {
		rc = offset;
		goto write_mutex_unlock;
	}

	//Write body
	offset = et_vqueue_buffer_write(sq_buf, offset,
					vqueue->vqueue_common->queue_buf_size,
					buf, count);
	if (offset < 0) {
		rc = offset;
		goto write_mutex_unlock;
	}

	head_index = (head_index + 1U) % vqueue->vqueue_common->queue_buf_count;

	//Do a dummy read so that any PCIe writes still in flight are forced to
	//complete.
	tail_index = ioread16(&vqueue->vqueue_info->sq_tail);

	//Write the head index. The SoC may poll for the head_index changing; it
	//is critical that all data that goes with the index change be in memory
	//by the time the head index changes (see above read).
	iowrite16(head_index, &vqueue->vqueue_info->sq_head);

	//Read again to be assured head_index is done being updated.
	//TODO: write ordering is assured. Confirm and remove dummy reads.
	tail_index = ioread16(&vqueue->vqueue_info->sq_tail);

	//Notify the SoC new data is ready. Again, the head index change needs
	//to be assured to be in SoC mem before the IPI is fired.

	interrupt_vqueue(vqueue->vqueue_common->interrupt_addr);

	//Flush writes to PCIe
	ioread8(&vqueue->vqueue_info->slave_status);

	mutex_unlock(&vqueue->write_mutex);

	mutex_lock(&vqueue->buf_count_mutex);
	mutex_lock(&vqueue->threshold_mutex);
	mutex_lock(&vqueue->sq_bitmap_mutex);

	vqueue->available_buf_count -= 1;
	if (vqueue->available_buf_count < vqueue->available_threshold)
		vqueue->vqueue_common->sq_bitmap &= ~((u32)1 << vqueue->index);

	mutex_unlock(&vqueue->sq_bitmap_mutex);
	mutex_unlock(&vqueue->threshold_mutex);
	mutex_unlock(&vqueue->buf_count_mutex);

	return count;

write_mutex_unlock:
	mutex_unlock(&vqueue->write_mutex);
	return rc;
}

ssize_t et_vqueue_write_from_user(struct et_vqueue *vqueue,
				  const char __user *buf, size_t count)
{
	u8 *kern_buf;
	ssize_t rc;

	if (count > vqueue->vqueue_common->queue_buf_size) {
		pr_err("message too big (size %ld, max %d)", count,
		       vqueue->vqueue_common->queue_buf_size);
		return -EINVAL;
	}

	kern_buf = kzalloc(count, GFP_KERNEL);

	if (!kern_buf)
		return -ENOMEM;

	rc = copy_from_user(kern_buf, buf, count);
	if (rc) {
		pr_err("copy_from_user failed\n");
		rc = -ENOMEM;
		goto error;
	}

	rc = et_vqueue_write(vqueue, kern_buf, count);

error:
	kfree(kern_buf);

	return rc;
}

/*
 * If there's a VQ message available, return 1 otherwise return 0.
 */
bool usr_message_available(struct et_vqueue *vqueue, struct et_msg_node **msg)
{
	int rv = 0;

	mutex_lock(&vqueue->msg_list_mutex);

	*msg = list_first_entry_or_null(&vqueue->msg_list, struct et_msg_node,
					list);
	rv = !!(*msg);

	mutex_unlock(&vqueue->msg_list_mutex);

	return rv;
}

ssize_t et_vqueue_read_to_user(struct et_vqueue *vqueue, char __user *buf,
			       size_t count)
{
	struct et_msg_node *msg = NULL;
	struct cmn_header_t *dev_ops_api_header = NULL;
	struct et_dma_info *dma_info = NULL;
	struct et_vqueue_common *vq_common = vqueue->vqueue_common;

	if (vqueue->flags & VQUEUE_FLAG_ABORT)
		return -EINTR;

	if (!buf || count == 0)
		return -EINVAL;

	if (!usr_message_available(vqueue, &msg)) {
		mutex_lock(&vqueue->cq_bitmap_mutex);
		vq_common->cq_bitmap &= ~((u32)1 << vqueue->index);
		mutex_unlock(&vqueue->cq_bitmap_mutex);
		pr_err("VQ[%d]: empty; no message to pop\n", vqueue->index);
		return -EAGAIN;
	}

	if (!msg)
		return -EINVAL;

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		return -ENOMEM;
	}

	if (!(msg->msg))
		return -EINVAL;

	dev_ops_api_header = (struct cmn_header_t *)msg->msg;

	if (dev_ops_api_header->msg_id ==
	    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP ||
	    dev_ops_api_header->msg_id ==
	    DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP) {
		mutex_lock(&vq_common->dma_rbtree_mutex);
		dma_info = et_dma_search_info(&vq_common->dma_rbtree,
					      dev_ops_api_header->tag_id);
		if (dev_ops_api_header->msg_id ==
		    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
			if (dma_info && copy_to_user(dma_info->usr_vaddr,
						     dma_info->kern_vaddr,
						     dma_info->size))
				pr_err("failed to copy DMA buffer\n");
		}
		et_dma_delete_info(&vq_common->dma_rbtree, dma_info);
		mutex_unlock(&vq_common->dma_rbtree_mutex);
	}

	if (copy_to_user(buf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		return -ENOMEM;
	}

	mutex_lock(&vqueue->msg_list_mutex);
	list_del(&msg->list);
	mutex_unlock(&vqueue->msg_list_mutex);

	destroy_msg_node(msg);

	return msg->msg_size;
}

bool et_vqueue_header_valid(struct et_vqueue *vqueue,
			    struct et_vqueue_header *header)
{
	if (header->length < ET_DEV_OPS_API_HEADER_SIZE)
		return false;
	if (header->length > vqueue->vqueue_common->queue_buf_size)
		return false;
	if (header->magic != ET_VQUEUE_MAGIC)
		return false;
	return true;
}

static int handle_msg(struct et_vqueue *vqueue, void __iomem *buf_to_read)
{
	ssize_t offset;
	struct et_vqueue_common *vq_common = vqueue->vqueue_common;
	struct et_vqueue_header header;
	struct cmn_header_t dev_ops_api_header;
	struct et_msg_node *msg_node;

	//Caller assures at least header and message ID are available

	if (!buf_to_read) {
		pr_err("NULL buf_to_read pointer being used in %s\n",
		       __func__);
		return -1;
	}

	/* TODO SW-4970: Remove the extra header */
	//Read the message header
	offset = 0;
	offset = et_vqueue_buffer_read(buf_to_read, offset,
				       vq_common->queue_buf_size,
				       (u8 *)&header, ET_VQUEUE_HEADER_SIZE);
	if (offset < 0)
		return offset;

	//If the header is invalid, the message buffer is corrupt and
	//the system is in a bad state. This should never happen.
	if (!et_vqueue_header_valid(vqueue, &header)) {
		iowrite8(VQUEUE_STATUS_ERROR,
			 &vqueue->vqueue_info->slave_status);
		interrupt_vqueue(vq_common->interrupt_addr);
		panic("VQueue corrupt");
		return -1;
	}

	//Read the message ID
	offset = et_vqueue_buffer_read(buf_to_read, offset,
				       vq_common->queue_buf_size,
				       (u8 *)&dev_ops_api_header,
				       ET_DEV_OPS_API_HEADER_SIZE);
	if (offset < 0)
		return offset;

	//Dispatch based on ID. Messages for the kernel are handled right away,
	//messages for user mode are saved off for the user to fetch at their
	//leisure.
	if (dev_ops_api_header.msg_id > DEV_OPS_API_MID_NONE &&
	    dev_ops_api_header.msg_id < DEV_OPS_API_MID_LAST) {
		//Message is for user mode. Save it off.
		msg_node = create_msg_node(header.length);

		if (!msg_node)
			return -1;
		memcpy(msg_node->msg, (u8 *)&dev_ops_api_header,
		       ET_DEV_OPS_API_HEADER_SIZE);

		//MMIO msg body directly into node memory
		offset =
		et_vqueue_buffer_read(buf_to_read, offset,
				      vqueue->vqueue_common->queue_buf_size,
				      (u8 *)msg_node->msg +
				      ET_DEV_OPS_API_HEADER_SIZE,
				      header.length -
				      ET_DEV_OPS_API_HEADER_SIZE);
		if (offset < 0)
			return offset;

		mutex_lock(&vqueue->msg_list_mutex);
		list_add_tail(&msg_node->list, &vqueue->msg_list);
		mutex_unlock(&vqueue->msg_list_mutex);

		mutex_lock(&vqueue->buf_count_mutex);
		vqueue->available_buf_count += 1;
		mutex_unlock(&vqueue->buf_count_mutex);

		wake_up_interruptible(&vqueue->vqueue_common->vqueue_wq);
	}

	return 0;
}

static void et_vqueue_handshaking(struct et_vqueue *vqueue)
{
	u8 master_status, slave_status;

	master_status = ioread8(&vqueue->vqueue_info->master_status);
	slave_status = ioread8(&vqueue->vqueue_info->slave_status);

	//The host is always the vqueue slave
	switch (master_status) {
	case VQUEUE_STATUS_NOT_READY:
		break;
	case VQUEUE_STATUS_READY:
		if (slave_status != VQUEUE_STATUS_READY &&
		    slave_status != VQUEUE_STATUS_WAITING) {
			pr_info
			("vq[%d]: received master ready, going slave ready",
			vqueue->index);
			iowrite8(VQUEUE_STATUS_READY,
				 &vqueue->vqueue_info->slave_status);

			//Flush PCIe write
			slave_status = ioread8
					(&vqueue->vqueue_info->slave_status);

			interrupt_vqueue(vqueue->vqueue_common->interrupt_addr);

			//Flush interrupt IPI write
			slave_status = ioread8
					(&vqueue->vqueue_info->slave_status);
		}
		break;
	case VQUEUE_STATUS_WAITING:
		if (slave_status != VQUEUE_STATUS_READY &&
		    slave_status != VQUEUE_STATUS_WAITING) {
			pr_info
			("vq[%d]: received master waiting, going slave ready",
			vqueue->index);
			iowrite8(VQUEUE_STATUS_READY,
				 &vqueue->vqueue_info->slave_status);

			//Flush PCIe write
			slave_status = ioread8
					(&vqueue->vqueue_info->slave_status);

			interrupt_vqueue(vqueue->vqueue_common->interrupt_addr);

			//Flush interrupt IPI write
			slave_status = ioread8
					(&vqueue->vqueue_info->slave_status);
		}
		break;
	case VQUEUE_STATUS_ERROR:
		break;
	}

	vqueue->is_ready = master_status == VQUEUE_STATUS_READY &&
				slave_status == VQUEUE_STATUS_READY;

	if (vqueue->is_ready) {
		pr_info("vq[%d]: is ready now", vqueue->index);
		wake_up_interruptible(&vqueue->vqueue_common->vqueue_wq);
	}
}

/*
 * Handles vqueue IRQ. The vqueue IRQ signals a state change and/or a new
 * message in the CQ.
 *
 * This method handles CQ messages for the kernel immediatley, and saves
 * off messages for user mode to be consumed later.
 *
 * User mode messages must not block kernel messages from being processed
 * (e.g if the first msg in the vqueue is for the user and the second is for
 * the kernel, the kernel message should not be stuck in line behind the user
 * message).
 *
 * This method must be tolerant of spurious IRQs (no state change or new msg),
 * and taking an IRQ while messages are still in filght.
 *
 * Reasons it may fire:
 *
 * - The host sent a state update and/or msg to this vqueue
 *
 * - The host sent a state update and/or msg to another vqueue, and MSI
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
void et_vqueue_isr_bottom(struct et_vqueue *vqueue)
{
	u16 head_index, tail_index;
	u16 buffer_avail;
	void __iomem *queue;
	void __iomem *buf_to_read;
	bool got_msg = false;
	int rv;

	if (!vqueue) {
		pr_err("NULL vqueue pointer passed to %s\n", __func__);
		return;
	}
	if (!vqueue->vqueue_buf) {
		pr_err("NULL vqueue_buf pointer passed to %s\n", __func__);
		return;
	}

	queue = vqueue->vqueue_buf->cq_buf;

	// TODO: check et_vqueue_handshaking instead of et_vqueue_ready
	// since device can also reset/change state of queue(s). And device
	// will send interrupt to host after changing vqueue status. So inside
	// bottom half, we should be reading Master (device) status from shared
	// area instead of relying on host only state variable.
	if (!et_vqueue_ready(vqueue)) {
		et_vqueue_handshaking(vqueue);
		return;
	}

	mutex_lock(&vqueue->read_mutex);

	head_index = ioread16(&vqueue->vqueue_info->cq_head);
	tail_index = ioread16(&vqueue->vqueue_info->cq_tail);

	buffer_avail = CIRC_CNT(head_index, tail_index,
				vqueue->vqueue_common->queue_buf_count);

	//Handle all pending messages in the vqueue
	while (buffer_avail >= 1) {
		if (!queue)
			goto read_mutex_unlock;

		buf_to_read =
		queue + tail_index * vqueue->vqueue_common->queue_buf_size;

		rv = handle_msg(vqueue, buf_to_read);

		if (!rv) {
			tail_index = (tail_index + 1U) %
					vqueue->vqueue_common->queue_buf_count;
			got_msg = true;
		} else {
			break;
		}

		//Check if there are more messages
		head_index = ioread16(&vqueue->vqueue_info->cq_head);
		buffer_avail = CIRC_CNT(head_index, tail_index,
					vqueue->vqueue_common->queue_buf_count);
	}

	if (got_msg) {
		//Return the buffer space read to the SoC
		iowrite16(tail_index, &vqueue->vqueue_info->cq_tail);

		//Read to make sure the write has propagated through PCIe
		//TODO: is this really necessary?
		tail_index = ioread16(&vqueue->vqueue_info->cq_tail);
	}

read_mutex_unlock:
	mutex_unlock(&vqueue->read_mutex);
}
