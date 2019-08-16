#include "et_ringbuffer.h"

#include <linux/io.h>

uint32_t et_ringbuffer_used(uint32_t head_index, uint32_t tail_index)
{
	return (head_index >= tail_index) ?
		       head_index - tail_index :
		       (ET_RINGBUFFER_LENGTH + head_index - tail_index);
}

uint32_t et_ringbuffer_free(const uint32_t head_index,
			    const uint32_t tail_index)
{
	return (head_index >= tail_index) ?
		       (ET_RINGBUFFER_LENGTH - 1U) - (head_index - tail_index) :
		       tail_index - head_index - 1U;
}

uint32_t et_ringbuffer_write(void __iomem *queue, uint8_t *buff,
			     uint32_t head_index, size_t len)
{
	uint32_t dwords;

	//Write until next u32 alignment
	while (head_index & 0x3 && len) {
		iowrite8(*buff, queue + head_index);
		head_index = (head_index + 1U) % ET_RINGBUFFER_LENGTH;
		++buff;
		--len;
	}

	//Write u32 aligned values
	dwords = len / 4;

	while (dwords) {
		iowrite32(*(u32 *)buff, queue + head_index);
		head_index = (head_index + 4U) % ET_RINGBUFFER_LENGTH;
		buff += 4;
		len -= 4;
		--dwords;
	}

	//Write any remaining bytes (0-3 bytes)
	while (len) {
		iowrite8(*buff, queue + head_index);
		head_index = (head_index + 1U) % ET_RINGBUFFER_LENGTH;
		++buff;
		--len;
	}

	return head_index;
}

uint32_t et_ringbuffer_read(void __iomem *queue, uint8_t *buff,
			    uint32_t tail_index, size_t len)
{
	uint32_t dwords;

	//Read until next u32 alignment
	while (tail_index & 0x3 && len) {
		*buff = ioread8(queue + tail_index);
		tail_index = (tail_index + 1U) % ET_RINGBUFFER_LENGTH;
		++buff;
		--len;
	}

	//Read u32 aligned values
	dwords = len / 4;

	while (dwords) {
		*(u32 *)buff = ioread32(queue + tail_index);
		tail_index = (tail_index + 4U) % ET_RINGBUFFER_LENGTH;
		buff += 4;
		len -= 4;
		--dwords;
	}

	//Read any remaining bytes (0-3 bytes)
	while (len) {
		*buff = ioread8(queue + tail_index);
		tail_index = (tail_index + 1U) % ET_RINGBUFFER_LENGTH;
		++buff;
		--len;
	}

	return tail_index;
}
