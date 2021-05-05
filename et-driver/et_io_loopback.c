#include "et_io.h"

void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count)
{
	memcpy(dst + offset, src, count);
}

void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count)
{
	memcpy(dst, src + offset, count);
}
