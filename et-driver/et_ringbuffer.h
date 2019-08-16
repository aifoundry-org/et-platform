#ifndef __ET_RINGBUFFER_H
#define __ET_RINGBUFFER_H

#include <linux/types.h>

// Make length be a factor of eight to facilitate alignment (2032 bytes)
#define ET_RINGBUFFER_LENGTH (254 * 8)
#define ET_RINGBUFFER_MAX_LENGTH (ET_RINGBUFFER_LENGTH - 1U)

struct et_ringbuffer {
	uint32_t head_index;
	uint32_t tail_index;
	uint8_t queue[ET_RINGBUFFER_LENGTH];
} __attribute__((__packed__));

uint32_t et_ringbuffer_used(uint32_t head_index, uint32_t tail_index);
uint32_t et_ringbuffer_free(const uint32_t head_index,
			    const uint32_t tail_index);
uint32_t et_ringbuffer_write(void __iomem *queue, uint8_t *buff,
			     uint32_t head_index, size_t len);
uint32_t et_ringbuffer_read(void __iomem *queue, uint8_t *buff,
			    uint32_t tail_index, size_t len);

#endif