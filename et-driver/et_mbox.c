#include "et_mbox.h"

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

enum et_mbox_status
{
	MBOX_STATUS_NOT_READY = 0U,
	MBOX_STATUS_READY = 1U,
	MBOX_STATUS_WAITING = 2U,
	MBOX_STATUS_ERROR = 3U
} __attribute__ ((__packed__));

#define ET_MBOX_MAGIC 0xBEEF

void et_mbox_init(struct et_mbox *mbox, void __iomem *mem,
		  void __iomem *r_pu_trg_pcie,
		  void (*send_interrupt)(void __iomem *trig_regs))
{
	volatile uint32_t temp;

	mbox->mem = (struct et_mbox_mem *)mem;
	mbox->r_pu_trg_pcie = r_pu_trg_pcie;
	mbox->send_interrupt = send_interrupt;

	//The host is always the mailbox slave; the SoC inits other fields
	iowrite32(MBOX_STATUS_NOT_READY, &mbox->mem->slave_status);

	//Flush writes to PCIe
	temp = ioread32(&mbox->mem->slave_status);

	//Notify SoC status changed
	if (mbox->send_interrupt) {
		mbox->send_interrupt(mbox->r_pu_trg_pcie);

		//Flush writes to PCIe
		temp = ioread32(&mbox->mem->slave_status);
	}
}

void et_mbox_destroy(struct et_mbox *mbox)
{
	uint32_t temp;

	if (mbox->mem) {
		//Request that the SoC reset the mailbox
		iowrite32(MBOX_STATUS_WAITING, &mbox->mem->slave_status);

		//Flush writes to PCIe
		temp = ioread32(&mbox->mem->slave_status);
	}

	mbox->mem = NULL;
	mbox->send_interrupt = NULL;
}

bool et_mbox_ready(struct et_mbox *mbox)
{
	uint32_t master_status, slave_status;

	master_status = ioread32(&mbox->mem->master_status);
	slave_status = ioread32(&mbox->mem->slave_status);

	return master_status == MBOX_STATUS_READY &&
	       slave_status == MBOX_STATUS_READY;
}

void et_mbox_reset(struct et_mbox *mbox)
{
	uint32_t slave_status;
	//Request that the SoC reset the mailboxes
	iowrite32(MBOX_STATUS_WAITING, &mbox->mem->slave_status);

	//Flush PCIe writes: this makes sure the status changes are visible
	//to the interrupt routines in the MM and SP
	slave_status = ioread32(&mbox->mem->slave_status);

	// Notify the masters
	if (mbox->send_interrupt) {
		mbox->send_interrupt(mbox->r_pu_trg_pcie);

		//Flush PCIe writes again for complete safety: the send_interrupt()
		//routine is really just an iowrite32(), flush them as well
		slave_status = ioread32(&mbox->mem->slave_status);
	}
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

	//TODO: grab mbox->write_mutex

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

	//Notify the SoC new data is ready, if it's not polling. Again, the head
	//index change needs to be assured to be in SoC mem before the IPI is
	//fired.
	if (mbox->send_interrupt) {
		mbox->send_interrupt(mbox->r_pu_trg_pcie);

		//Flush writes to PCIe
		temp = ioread32(&mbox->mem->slave_status);
	}

	//TODO: release mbox->write_mutex

	return count;
}

ssize_t et_mbox_write_from_user(struct et_mbox *mbox, const char __user *buf,
				size_t count)
{
	uint8_t *kern_buf;
	ssize_t rc;

	if (count > ET_MBOX_MAX_MSG_SIZE) {
		pr_err("message too big (size %ld, max %ld)", count,
		       ET_MBOX_MAX_MSG_SIZE);
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

ssize_t et_mbox_read_to_user(struct et_mbox *mbox, char __user *buf,
			     size_t count)
{
	uint8_t *kern_buf;
	ssize_t rc;
	size_t kern_buff_size = min(count, (size_t)(ET_MBOX_MAX_MSG_SIZE));

	kern_buf = kzalloc(kern_buff_size, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("kalloc failed\n");
		return -ENOMEM;
	}

	rc = et_mbox_read(mbox, kern_buf, count);

	if (rc > 0) {
		if (copy_to_user(buf, kern_buf, rc)) {
			pr_err("failed to copy to user\n");
			rc = -ENOMEM;
			goto error;
		}
	}

error:
	kfree(kern_buf);

	return rc;
}

ssize_t et_mbox_read(struct et_mbox *mbox, void* buff, size_t count)
{
	uint32_t head_index, tail_index;
	uint32_t bytes_avail;
	struct et_mbox_header header;
	void __iomem *queue = mbox->mem->tx_ring_buffer.queue;

	if (!et_mbox_ready(mbox)) {
		pr_err("mbox not ready\n");
		return -EIO;
	}

	//TODO: acquire mbox->read_mutex

	head_index = ioread32(&mbox->mem->tx_ring_buffer.head_index);
	tail_index = ioread32(&mbox->mem->tx_ring_buffer.tail_index);

	bytes_avail = et_ringbuffer_used(head_index, tail_index);

	//Wait for there to be a message in the mailbox before reading
	if (bytes_avail < ET_MBOX_HEADER_SIZE) {
		//TODO: block in this condition. Sleep for IRQ to signal to
		//check again. For now, just return 0 (no bytes read - not
		//an error condition.
		return 0;
	}

	//Read the message header
	tail_index = et_ringbuffer_read(queue, (uint8_t *)&header, tail_index,
					ET_MBOX_HEADER_SIZE);

	//Check if the message is valid
	if (header.length < 1 ||
	    header.length > ET_MBOX_MAX_MSG_SIZE ||
	    header.magic != ET_MBOX_MAGIC) {
		//If the header is invalid, remove it from the queue
		tail_index = (tail_index + ET_MBOX_HEADER_SIZE) %
			     ET_RINGBUFFER_LENGTH;

		iowrite32(tail_index, &mbox->mem->tx_ring_buffer.tail_index);

		pr_err("invalid mailbox message (header: 0x%08x)\n",
		       *((u32 *)&header));
		return -EIO;
	}

	//Check if the buffer is big enough to store the message body
	if (count < header.length) {
		//If not, do NOT remove the message from the queue. Give the
		//user a chance to allocate a bigger buffer and try again.
		return -ENOMEM;
	}

	//Check if the body of the message is available yet
	if (bytes_avail - ET_MBOX_HEADER_SIZE < header.length) {
		//TODO: Once mbox IRQs are implemented, this is an error
		//condition - the SoC should not send the interrupt until the
		//message body data is in the buffer. For now, return 0 (without
		//moving tail_index!!!) in this case. The user should call
		//read() in a loop until the whole message is available.
		return 0;
	}

	//Read message body over PCIe
	tail_index = et_ringbuffer_read(queue, buff, tail_index, header.length);

	//Return the buffer space read to the SoC
	iowrite32(tail_index, &mbox->mem->tx_ring_buffer.tail_index);

	//Read to make sure the write has propagated through PCIe
	tail_index = ioread32(&mbox->mem->tx_ring_buffer.tail_index);

	//TODO: release mbox->read_mutex

	return header.length;
}

void et_mbox_isr(struct et_mbox *mbox)
{
	uint32_t master_status, slave_status;

	//The SoC will interrupt when it sends data, or changes status

	master_status = ioread32(&mbox->mem->master_status);
	slave_status = ioread32(&mbox->mem->slave_status);

	//The host is always the mbox slave
	switch (master_status) {
	case MBOX_STATUS_NOT_READY:
		break;
	case MBOX_STATUS_READY:
		if (slave_status != MBOX_STATUS_READY &&
		    slave_status != MBOX_STATUS_WAITING) {
			//received master ready, going slave ready
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
			//received master waiting, going slave ready
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

	//TODO: signal any blocked read() calls if mbox ready
}