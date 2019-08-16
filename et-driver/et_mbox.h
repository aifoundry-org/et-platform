#ifndef __ET_MBOX_H
#define __ET_MBOX_H

#include <linux/types.h>
#include "et_ringbuffer.h"

struct et_mbox_mem
{
	uint32_t master_status;
	uint32_t slave_status;
	struct et_ringbuffer tx_ring_buffer;
	struct et_ringbuffer rx_ring_buffer;
} __attribute__ ((__packed__));

struct et_mbox {
	struct et_mbox_mem *mem;
	void __iomem *r_pu_trg_pcie;
	void (*send_interrupt)(void __iomem *r_pu_trg_pcie);
};

struct et_mbox_header
{
	uint16_t length;
	uint16_t magic;
} __attribute__ ((__packed__));

#define ET_MBOX_HEADER_SIZE (sizeof(struct et_mbox_header))

#define ET_MBOX_MAX_MSG_SIZE (ET_RINGBUFFER_MAX_LENGTH - ET_MBOX_HEADER_SIZE)

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

void et_mbox_isr(struct et_mbox *mbox);

#endif