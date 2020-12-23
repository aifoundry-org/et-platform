#include "et_ringbuffer.h"
#include "et_io.h"

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
	size_t bytes_to_write;

	while (len) {
		if ((head_index + len) > ET_RINGBUFFER_LENGTH)
			bytes_to_write = ET_RINGBUFFER_LENGTH - head_index;
		else
			bytes_to_write = len;

		et_iowrite(queue, head_index, buff, bytes_to_write);

		head_index = (head_index + bytes_to_write) %
			     ET_RINGBUFFER_LENGTH;
		len -= bytes_to_write;
		buff += bytes_to_write;
	}

	return head_index;
}

uint32_t et_ringbuffer_read(void __iomem *queue, uint8_t *buff,
			    uint32_t tail_index, size_t len)
{
	size_t bytes_to_read;

	while (len) {
		if ((tail_index + len) > ET_RINGBUFFER_LENGTH)
			bytes_to_read = ET_RINGBUFFER_LENGTH - tail_index;
		else
			bytes_to_read = len;

		et_ioread(queue, tail_index, buff, bytes_to_read);

		tail_index = (tail_index + bytes_to_read) %
			     ET_RINGBUFFER_LENGTH;
		len -= bytes_to_read;
		buff += bytes_to_read;
	}

	return tail_index;
}
