#ifndef __ET_IOCTL_H
#define __ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESPERANTO_PCIE_IOCTL_MAGIC	0xE7

#define CMD_DESC_FLAG_DMA		(0x1U)

struct mmio_desc {
	void *ubuf;
	__u64 size;
	__u64 devaddr;
};

struct cmd_desc {
	void *cmd;
	__u16 size;
	__u16 sq_index;
	__u8 flags;
};

struct rsp_desc {
	void *rsp;
	__u16 size;
	__u16 cq_index;
};

struct sq_threshold {
	__u16 bytes_needed;
	__u16 sq_index;
};

struct dram_info {
	__u64 base;
	__u64 size;
	__u16 align_in_bits;
};

#define ETSOC1_IOCTL_GET_USER_DRAM_INFO				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, struct dram_info)

#define ETSOC1_IOCTL_MMIO_WRITE					\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 2, struct mmio_desc)

#define ETSOC1_IOCTL_MMIO_READ					\
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 3, struct mmio_desc)

#define ETSOC1_IOCTL_GET_SQ_COUNT				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 4, __u16)

#define ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 5, __u16)

#define ETSOC1_IOCTL_GET_ACTIVE_SHIRE				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 6, __u64)

#define ETSOC1_IOCTL_PUSH_SQ					\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 7, struct cmd_desc)

#define ETSOC1_IOCTL_POP_CQ					\
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 8, struct rsp_desc)

#define ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 9, __u64)

#define ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 10, __u64)

#define ETSOC1_IOCTL_SET_SQ_THRESHOLD				\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 11, struct sq_threshold)

#define ETSOC1_IOCTL_TEST_VQ					\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 12, __u16)

#endif
