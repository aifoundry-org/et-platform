#include "et_mmio.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

int et_mmio_iomem_idx(uint64_t soc_addr, uint64_t count)
{
	int i;
	uint64_t reg_begin, reg_end;
	uint64_t soc_end = soc_addr + count;

	//integer overflow check
	if (soc_end < soc_addr) {
		pr_err("Invalid soc_addr + size (0x%010llx + 0x%llx)\n",
		       soc_addr, count);
		return -EINVAL;
	}

	for (i = 0; i < IOMEM_REGIONS; ++i) {
		reg_begin = BAR_MAPPINGS[i].soc_addr;
		reg_end = reg_begin + BAR_MAPPINGS[i].size;

		if (soc_addr >= reg_begin && soc_end <= reg_end) {
			return i;
		}
	}

	//Didn't find any regions matching parameters
	return -EINVAL;
}

static void et_iowrite8_block(void __iomem *port, const void *src, u64 count)
{
	while (count--) {
		iowrite8(*(u8 *)src, port);
		++src;
		++port;
	}
}

static void et_iowrite32_block(void __iomem *port, const void *src, u64 count)
{
	while (count--) {
		iowrite32(*(u32 *)src, port);
		src += 4;
		port += 4;
	}
}

ssize_t et_mmio_write_from_user(const char __user *buf, size_t count,
				loff_t *pos, struct et_pci_dev *et_dev)
{
	int iomem_idx;
	ssize_t rv, write_count = 0;
	void __iomem *iomem;
	uint64_t soc_addr = (uint64_t)*pos;
	uint64_t off;
	uint64_t iocount;
	uint8_t *kern_buf;
	uint8_t *buf_pos;

	//Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Pull user's buffer to kernel
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}
	buf_pos = kern_buf;

	rv = copy_from_user(kern_buf, buf, count);
	if (rv) {
		pr_err("failed to copy from user\n");
		goto error;
	}

	//Write initial bytes to get to u32 alignment
	iocount = 4 - (off & 0x3);
	if (iocount) {
		et_iowrite8_block(iomem + off, buf_pos, iocount);

		off += iocount;
		buf_pos += iocount;
		write_count += iocount;
	}

	//Copy 32-bit aligned values
	//TODO: enable/use 64-bit MMIO accesses
	iocount = (count - write_count) / 4;
	if (iocount) {
		et_iowrite32_block(iomem + off, buf_pos, iocount);

		off += iocount * 4;
		buf_pos += iocount * 4;
		write_count += iocount * 4;
	}

	//Remaining bytes smaller than a u32
	iocount = count - write_count;
	if (iocount) {
		et_iowrite8_block(iomem + off, buf_pos, iocount);

		write_count += iocount;
	}

	rv = write_count;
	*pos += write_count;
error:
	kfree(kern_buf);

	return rv;
}

static void et_ioread8_block(void __iomem *port, const void *dst, u64 count)
{
	while (count--) {
		*(u8 *)dst = ioread8(port);
		++port;
		++dst;
	}
}

static void et_ioread32_block(void __iomem *port, const void *dst, u64 count)
{
	while (count--) {
		*(u32 *)dst = ioread32(port);
		dst += 4;
		port += 4;
	}
}

ssize_t et_mmio_read_to_user(char __user *buf, size_t count, loff_t *pos,
			     struct et_pci_dev *et_dev)
{
	int iomem_idx;
	ssize_t rv, read_count = 0;
	void __iomem *iomem;
	uint64_t soc_addr = (uint64_t)*pos;
	uint64_t off;
	uint64_t iocount;
	uint8_t *kern_buf;
	uint8_t *buf_pos;

	//Bounds check and lookup BAR mapping
	iomem_idx = et_mmio_iomem_idx(soc_addr, count);
	if (iomem_idx < 0) {
		return iomem_idx;
	}
	iomem = et_dev->iomem[iomem_idx];
	off = soc_addr - BAR_MAPPINGS[iomem_idx].soc_addr;

	//Buffer incoming data
	kern_buf = kmalloc(count, GFP_KERNEL);
	if (!kern_buf) {
		pr_err("failed to kmalloc\n");
		return -ENOMEM;
	}
	buf_pos = kern_buf;

	//Go to next u32 aligned iomem 
	iocount = 4 - (off & 0x3);
	if (iocount) {
		et_ioread8_block(iomem + off, buf_pos, iocount);

		off += iocount;
		buf_pos += iocount;
		read_count += iocount;
	}

	//Copy 32-bit aligned values.
	//TODO: enable/use 64-bit MMIO accesses
	iocount = (count - read_count) / 4;
	if (iocount) {
		et_ioread32_block(iomem + off, buf_pos, iocount);

		off += iocount * 4;
		buf_pos += iocount * 4;
		read_count += iocount * 4;
	}

	//Remaining bytes smaller than a u32
	iocount = count - read_count;
	if (iocount) {
		et_ioread8_block(iomem + off, buf_pos, iocount);

		read_count += iocount;
	}

	rv = copy_to_user(buf, kern_buf, read_count);
	if (rv) {
		pr_err("failed to copy to user\n");
		goto error;
	}

	rv = read_count;

error:
	kfree(kern_buf);

	return rv;
}
