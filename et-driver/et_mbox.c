#include "et_mbox.h"

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "et_pci_dev.h"

enum et_mbox_status
{
	MBOX_STATUS_NOT_READY = 0U,
	MBOX_STATUS_READY = 1U,
	MBOX_STATUS_WAITING = 2U,
	MBOX_STATUS_ERROR = 3U
} __attribute__ ((__packed__));

#define ET_MBOX_MAGIC 0xBEEF

static struct et_msg_node *create_msg_node(uint32_t msg_size)
{
	struct et_msg_node *new_node;

	//Build node
	new_node = kmalloc(sizeof(struct et_msg_node), GFP_KERNEL);

	if (IS_ERR(new_node)) {
		panic("Failed to allocate msg node, error %ld\n", PTR_ERR(new_node));
	}

	new_node->msg = kmalloc(msg_size, GFP_KERNEL);

	if (IS_ERR(new_node->msg)) {
		panic("Failed to allocate msg buffer, error %ld\n", PTR_ERR(new_node->msg));
	}

	new_node->msg_size = msg_size;

	return new_node;
}

static void destroy_msg_node(struct et_msg_node *node)
{
	if (node->msg) {
		kfree(node->msg);
	}

	kfree(node);
}

static void destroy_msg_list(struct et_mbox *mbox)
{
	struct list_head *pos, *next;
	struct et_msg_node *node;

	list_for_each_safe(pos, next, &mbox->msg_list) {
		node = list_entry(pos, struct et_msg_node, list);
		list_del(pos);
		destroy_msg_node(node);
	}
}

void et_mbox_init(struct et_mbox *mbox, void __iomem *mem,
		  void __iomem *r_pu_trg_pcie,
		  void (*send_interrupt)(void __iomem *trig_regs))
{
	mbox->mem = (struct et_mbox_mem *)mem;
	mbox->r_pu_trg_pcie = r_pu_trg_pcie;
	mbox->send_interrupt = send_interrupt;

	mbox->flags = 0;
	
	mutex_init(&mbox->msg_list_mutex);
	mutex_init(&mbox->read_mutex);
	mutex_init(&mbox->write_mutex);

	INIT_LIST_HEAD(&mbox->msg_list);

	init_waitqueue_head(&mbox->user_msg_wq);

	//The host is always the mailbox slave; the SoC inits other fields
	iowrite32(MBOX_STATUS_NOT_READY, &mbox->mem->slave_status);

	et_mbox_reset(mbox);
}

void et_mbox_destroy(struct et_mbox *mbox)
{
	if (mbox->mem) {
		iowrite32(MBOX_STATUS_NOT_READY, &mbox->mem->slave_status);
	}

	mutex_lock(&mbox->msg_list_mutex);
	mbox->flags = MBOX_FLAG_ABORT;
	mutex_unlock(&mbox->msg_list_mutex);

	wake_up_interruptible_all(&mbox->user_msg_wq);

	mutex_destroy(&mbox->write_mutex);
	mutex_destroy(&mbox->read_mutex);
	mutex_destroy(&mbox->msg_list_mutex);

	destroy_msg_list(mbox);

	mbox->mem = NULL;
	mbox->r_pu_trg_pcie = NULL;
	mbox->send_interrupt = NULL;
}

bool et_mbox_ready(struct et_mbox *mbox)
{
	uint32_t master_status, slave_status;

	master_status = ioread32(&mbox->mem->master_status);
	slave_status = ioread32(&mbox->mem->slave_status);

	//The host is always the mbox slave
	switch (master_status) {
	case MBOX_STATUS_NOT_READY:
		break;
	case MBOX_STATUS_READY:
		if (slave_status != MBOX_STATUS_READY &&
		    slave_status != MBOX_STATUS_WAITING) {
			pr_info("received master ready, going slave ready");
			iowrite32(MBOX_STATUS_READY, &mbox->mem->slave_status);

			//Flush PCIe write
			slave_status = ioread32(&mbox->mem->slave_status);

			mbox->send_interrupt(mbox->r_pu_trg_pcie);

			//Flush interrupt IPI write
			slave_status = ioread32(&mbox->mem->slave_status);
		}
		break;
	case MBOX_STATUS_WAITING:
		if (slave_status != MBOX_STATUS_READY &&
		    slave_status != MBOX_STATUS_WAITING) {
			pr_info("received master waiting, going slave ready");
			iowrite32(MBOX_STATUS_READY, &mbox->mem->slave_status);

			//Flush PCIe write
			slave_status = ioread32(&mbox->mem->slave_status);

			mbox->send_interrupt(mbox->r_pu_trg_pcie);

			//Flush interrupt IPI write
			slave_status = ioread32(&mbox->mem->slave_status);
		}
		break;
	case MBOX_STATUS_ERROR:
		break;
	}

	return master_status == MBOX_STATUS_READY &&
	       slave_status == MBOX_STATUS_READY;
}

void et_mbox_reset(struct et_mbox *mbox)
{
	uint32_t slave_status;

	mutex_lock(&mbox->msg_list_mutex);
	mutex_lock(&mbox->read_mutex);
	mutex_lock(&mbox->write_mutex);

	destroy_msg_list(mbox);

	//Request that the SoC reset the mailboxes
	iowrite32(MBOX_STATUS_WAITING, &mbox->mem->slave_status);

	//Flush PCIe writes: this makes sure the status changes are visible
	//to the interrupt routines in the MM and SP
	slave_status = ioread32(&mbox->mem->slave_status);

	//Notify SoC status changed
	mbox->send_interrupt(mbox->r_pu_trg_pcie);

	//Flush PCIe writes again for complete safety: the send_interrupt()
	//routine is really just an iowrite32(), flush them as well
	slave_status = ioread32(&mbox->mem->slave_status);
	
	mutex_unlock(&mbox->write_mutex);
	mutex_unlock(&mbox->read_mutex);
	mutex_unlock(&mbox->msg_list_mutex);
}

ssize_t et_mbox_write(struct et_mbox *mbox, void *buff, size_t count)
{
	volatile uint32_t head_index, tail_index;
	uint32_t free_bytes, temp;
	void __iomem *queue = mbox->mem->rx_ring_buffer.queue;
	const struct et_mbox_header header = { .length = (uint16_t)count,
					       .magic = ET_MBOX_MAGIC };

	if (!et_mbox_ready(mbox)) {
		pr_err("mbox not ready\n");
		return -EIO;
	}

	if (count < 1 || count > U16_MAX) {
		return -EINVAL;
	}

	mutex_lock(&mbox->write_mutex);

	head_index = ioread32(&mbox->mem->rx_ring_buffer.head_index);
	tail_index = ioread32(&mbox->mem->rx_ring_buffer.tail_index);

	free_bytes = et_ringbuffer_free(head_index, tail_index);

	if (free_bytes < ET_MBOX_HEADER_SIZE + count) {
		pr_err("no room for message (%d free, %ld needed)\n",
		       free_bytes, ET_MBOX_HEADER_SIZE + count);
		return -ENOMEM;
	}

	//Write header
	head_index = et_ringbuffer_write(queue, (u8 *)&header, head_index,
					 ET_MBOX_HEADER_SIZE);

	//Write body
	head_index = et_ringbuffer_write(queue, buff, head_index, count);

	//Do a dummy read so that any PCIe writes still in flight are forced to
	//complete.
	tail_index = ioread32(&mbox->mem->rx_ring_buffer.tail_index);

	//Write the head index. The SoC may poll for the head_index changing; it
	//is critical that all data that goes with the index change be in memory
	//by the time the head index changes (see above read).
	iowrite32(head_index, &mbox->mem->rx_ring_buffer.head_index);

	//Read again to be assured head_index is done being updated. TODO: write ordering is assured. Confirm and remove dummy reads.
	tail_index = ioread32(&mbox->mem->rx_ring_buffer.tail_index);

	//Notify the SoC new data is ready. Again, the head index change needs
	//to be assured to be in SoC mem before the IPI is fired.
	
	mbox->send_interrupt(mbox->r_pu_trg_pcie);

	//Flush writes to PCIe
	temp = ioread32(&mbox->mem->slave_status);
	
	mutex_unlock(&mbox->write_mutex);

	return count;
}

ssize_t et_mbox_write_from_user(struct et_mbox *mbox, const char __user *buf,
				size_t count)
{
	uint8_t *kern_buf;
	ssize_t rc;

	if (count > ET_MBOX_MAX_MSG_LEN) {
		pr_err("message too big (size %ld, max %ld)", count,
		       ET_MBOX_MAX_MSG_LEN);
		return -EINVAL;
	}

	kern_buf = kzalloc(count, GFP_KERNEL);

	if (!kern_buf) {
		pr_err("kalloc failed\n");
		return -ENOMEM;
	}

	rc = copy_from_user(kern_buf, buf, count);
	if (rc) {
		pr_err("copy_from_user failed\n");
		rc = -ENOMEM;
		goto error;
	}

	rc = et_mbox_write(mbox, kern_buf, count);

error:
	kfree(kern_buf);

	return rc;
}

/* 
 * If there's a mailbox message available, pop it from the list and return 1,
 * otherwise return 0.
 */
static int pop_usr_msg(struct et_mbox *mbox, struct et_msg_node **msg,
		       uint32_t *flags)
{
	int rv = 0;

	mutex_lock(&mbox->msg_list_mutex);

	*flags = mbox->flags;
	if (*flags & MBOX_FLAG_ABORT) {
		mutex_unlock(&mbox->msg_list_mutex);
		return 1;
	}

	*msg = list_first_entry_or_null(&mbox->msg_list, struct et_msg_node,
					list);

	if (*msg) {
		list_del(&((*msg)->list));
		rv = 1;
	}

	mutex_unlock(&mbox->msg_list_mutex);

	return rv;
}

ssize_t et_mbox_read_to_user(struct et_mbox *mbox, char __user *buf,
			     size_t count)
{
	struct et_msg_node *msg;
	uint32_t flags;
	ssize_t rc;

	//Copy the next available message from the SoC to user mode
	wait_event_interruptible(mbox->user_msg_wq,
				 pop_usr_msg(mbox, &msg, &flags));

	if (flags & MBOX_FLAG_ABORT) return -EINTR;

	if (!msg) {
		//Can happen if user SIG_KILLs to interrupt the wait
		pr_err("Failed to pop a message\n");
		return -ENOMEM;
	}

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		rc = -ENOMEM;
		goto error;  //TODO: maybe don't delete message in this case...
	}

	if (copy_to_user(buf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		rc = -ENOMEM;
		goto error;  //TODO: maybe don't delete message in this case...
	}

	rc = msg->msg_size;

error:
	destroy_msg_node(msg);

	return rc;
}

bool et_mbox_header_valid(struct et_mbox_header *header)
{
	if (header->length < ET_MBOX_MIN_MSG_LEN) return false;
	if (header->length > ET_MBOX_MAX_MSG_LEN) return false;
	if (header->magic != ET_MBOX_MAGIC) return false;
	return true;
}

static int handle_msg(struct et_mbox *mbox, void __iomem *queue,
		      uint32_t *head_index, uint32_t *tail_index,
		      struct et_pci_dev *et_dev)
{
	struct et_mbox_header header;
	uint64_t msg_id;

	//Caller assures at least header and message ID are available

	//Read the message header
	*tail_index = et_ringbuffer_read(queue, (uint8_t *)&header, *tail_index,
					 ET_MBOX_HEADER_SIZE);

	//If the header is invalid, the message buffer is corrupt and
	//the system is in a bad state. This should never happen.
	if (!et_mbox_header_valid(&header)) {
		iowrite32(MBOX_STATUS_ERROR, &mbox->mem->slave_status);
		mbox->send_interrupt(mbox->r_pu_trg_pcie);
		panic("Mailbox corrupt");
	}

	//If the message is not all in the mbox, data is still in flight (we
	//likely took a spurrious IRQ). Wait to handle msg until the next IRQ
	if (et_ringbuffer_used(*head_index, *tail_index) < header.length) {
		*tail_index = (*tail_index - ET_MBOX_HEADER_SIZE) %
			      ET_RINGBUFFER_LENGTH;
		return -EAGAIN;
	}

	//Read the message ID
	*tail_index = et_ringbuffer_read(queue, (uint8_t *)&msg_id, *tail_index,
					 ET_MBOX_MSG_ID_SIZE);

	//Dispatch based on ID. Messages for the kernel are handled right away,
	//messages for user mode are saved off for the user to fetch at their
	//leisure.
	if (msg_id == MBOX_MESSAGE_ID_DMA_DONE) {
		//The SoC is signaling a DMA engine finished it's work. Wake
		//up the DMA channel the message is for.
		
		struct dma_done_message_t done_msg;
		enum et_dma_id dma_id;

		if (header.length != sizeof(done_msg) + ET_MBOX_MSG_ID_SIZE) {
			iowrite32(MBOX_STATUS_ERROR, &mbox->mem->slave_status);
			mbox->send_interrupt(mbox->r_pu_trg_pcie);
			panic("Mailbox corrupt (DMA Done size wrong)");
		}

		*tail_index = et_ringbuffer_read(queue, (uint8_t *)&done_msg,
						 *tail_index, sizeof(done_msg));

		dma_id = (enum et_dma_id)done_msg.chan;

		if (dma_id < ET_DMA_ID_READ_0 || dma_id > ET_DMA_ID_WRITE_3) {
			iowrite32(MBOX_STATUS_ERROR, &mbox->mem->slave_status);
			mbox->send_interrupt(mbox->r_pu_trg_pcie);
			panic("Mailbox corrupt (DMA Done chan invalid)");
		}

		mutex_lock(&et_dev->dma_chans[dma_id].state_mutex);
		if (done_msg.status == 0) {
			et_dev->dma_chans[dma_id].state = ET_DMA_STATE_DONE;
		} else {
			et_dev->dma_chans[dma_id].state = ET_DMA_STATE_ABORTED;
		}
		mutex_unlock(&et_dev->dma_chans[dma_id].state_mutex);

		wake_up_interruptible(&et_dev->dma_chans[dma_id].wait_queue);
	}
	else {
		//Message is for user mode. Save it off.
		struct et_msg_node *msg_node = create_msg_node(header.length);

		memcpy(msg_node->msg, (uint8_t *)&msg_id, ET_MBOX_MSG_ID_SIZE);

		//MMIO msg body directly into node memory
		*tail_index = et_ringbuffer_read(
			queue, msg_node->msg + ET_MBOX_MSG_ID_SIZE, *tail_index,
			header.length - ET_MBOX_MSG_ID_SIZE);

		mutex_lock(&mbox->msg_list_mutex);
		list_add_tail(&msg_node->list, &mbox->msg_list);
		mutex_unlock(&mbox->msg_list_mutex);

		wake_up_interruptible(&mbox->user_msg_wq);
	}

	return 0;
	//TODO: white list all messages? else case is an error? specific white list per mbox?
}

/*
 * Handles mbox IRQ. The mbox IRQ signals a state change and/or a new message
 * in the mailbox.
 * 
 * This method handles mailbox messages for the kernel immediatley, and saves
 * off messages for user mode to be consumed later.
 * 
 * User mode messages must not block kernel messages from being processed (e.x.
 * if the first msg in the mbox is for the user and the second is for the
 * kernel, the kernel message should not be stuck in line behind the user
 * message).
 * 
 * This method must be tolerant of spurrious IRQs (no state change or new msg),
 * and taking an IRQ while messages are still in filght.
 * 
 * Reasons it may fire:
 * 
 * - The host sent a state update and/or msg to this mbox
 * 
 * - The host sent a state update and/or msg to another mbox, and MSI
 *   multivector support is not available (IRQ is spurrious for this mbox)
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
 *   next IRQ), or there may be no changes (IRQ is spurrious)
 */
void et_mbox_isr_bottom(struct et_mbox *mbox, struct et_pci_dev *et_dev)
{
	uint32_t head_index, tail_index;
	uint32_t bytes_avail;
	void __iomem *queue = mbox->mem->tx_ring_buffer.queue;
	bool got_msg = false;
	int rv;

	//Check if ready, and also check for state updates
	if (!et_mbox_ready(mbox)) return;

	mutex_lock(&mbox->read_mutex);

	head_index = ioread32(&mbox->mem->tx_ring_buffer.head_index);
	tail_index = ioread32(&mbox->mem->tx_ring_buffer.tail_index);

	bytes_avail = et_ringbuffer_used(head_index, tail_index);

	//Handle all pending messages in the mbox
	while (bytes_avail >= ET_MBOX_MIN_MSG_SIZE)
	{
		rv = handle_msg(mbox, queue, &head_index, &tail_index, et_dev);

		if (rv < 0) break;
		
		got_msg = true;
		
		//Check if there are more messages
		head_index = ioread32(&mbox->mem->tx_ring_buffer.head_index);
		bytes_avail = et_ringbuffer_used(head_index, tail_index);		
	}

	if (got_msg) {
		//Return the buffer space read to the SoC
		iowrite32(tail_index, &mbox->mem->tx_ring_buffer.tail_index);

		//Read to make sure the write has propagated through PCIe
		//TODO: is this really necessary?
		tail_index = ioread32(&mbox->mem->tx_ring_buffer.tail_index);
	}

	mutex_unlock(&mbox->read_mutex);
}