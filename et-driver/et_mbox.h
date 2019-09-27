#ifndef __ET_MBOX_H
#define __ET_MBOX_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include "et_mbox_id.h"
#include "et_ringbuffer.h"

struct et_mbox_mem
{
	uint32_t master_status;
	uint32_t slave_status;
	struct et_ringbuffer tx_ring_buffer;
	struct et_ringbuffer rx_ring_buffer;
} __attribute__ ((__packed__));

struct et_mbox_header
{
	uint16_t length;
	uint16_t magic;
} __attribute__ ((__packed__));

struct et_msg_node {
	struct list_head list;
	uint8_t *msg;
	uint32_t msg_size;
};

#define MBOX_FLAG_ABORT 1

struct et_mbox {
	struct list_head msg_list;
	struct mutex msg_list_mutex;
	struct mutex read_mutex;
	struct mutex write_mutex;
	wait_queue_head_t user_msg_wq;
	struct et_mbox_mem *mem;
	void __iomem *r_pu_trg_pcie;
	void (*send_interrupt)(void __iomem *r_pu_trg_pcie);
	volatile long unsigned int flags;
};

struct dma_run_to_done_message_t {
	uint64_t message_id;
	uint32_t chan;
};

struct dma_done_message_t {
	uint32_t chan;
	int32_t status;
};

#define ET_MBOX_HEADER_SIZE ( sizeof(struct et_mbox_header) )
#define ET_MBOX_MSG_ID_SIZE ( sizeof(uint64_t) )

#define ET_MBOX_MIN_MSG_SIZE (ET_MBOX_HEADER_SIZE + ET_MBOX_MSG_ID_SIZE)

#define ET_MBOX_MIN_MSG_LEN (ET_MBOX_HEADER_SIZE)
#define ET_MBOX_MAX_MSG_LEN (ET_RINGBUFFER_MAX_LENGTH - ET_MBOX_HEADER_SIZE)

struct et_pci_dev;

void et_mbox_init(struct et_mbox *mbox, void __iomem *mem,
		  void __iomem *r_pu_trg_pcie,
		  void (*send_interrupt)(void __iomem *r_pu_trg_pcie));

void et_mbox_destroy(struct et_mbox *mbox);

bool et_mbox_ready(struct et_mbox *mbox);

void et_mbox_reset(struct et_mbox *mbox);

ssize_t et_mbox_write(struct et_mbox *mbox, void *buff, size_t count);

ssize_t et_mbox_write_from_user(struct et_mbox *mbox, const char __user *buf,
				size_t count);

ssize_t et_mbox_read(struct et_mbox *mbox, void *buff, size_t count);

ssize_t et_mbox_read_to_user(struct et_mbox *mbox, char __user *buf,
			     size_t count);

void et_mbox_isr_bottom(struct et_mbox *mbox, struct et_pci_dev *et_dev);

#endif