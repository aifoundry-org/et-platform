#ifndef __ET_IOCTL_H
#define __ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESPERANTO_PCIE_IOCTL_MAGIC	0xE7

#define CMD_INFO_FLAG_DMA		(0x1U)

typedef struct {
	void *ubuf;
	uint64_t size;
	uint64_t devaddr;
} mmio_info_t;

typedef struct {
	void *cmd;
	uint64_t size;
	uint8_t sq_index;
	uint8_t flags;
} cmd_info_t;

typedef struct {
	void *rsp;
	uint64_t size;
	uint8_t cq_index;
} rsp_info_t;

typedef struct {
	uint16_t count;
	uint8_t index;
} sq_availability_threshold_t;

#define ETSOC1_IOCTL_GET_USER_DRAM_BASE				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, uint64_t)

#define ETSOC1_IOCTL_GET_USER_DRAM_SIZE				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 2, uint64_t)

#define ETSOC1_IOCTL_MMIO_WRITE					\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 3, mmio_info_t)

#define ETSOC1_IOCTL_MMIO_READ					\
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 4, mmio_info_t)

#define ETSOC1_IOCTL_GET_VQ_COUNT				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 5, uint8_t)

#define ETSOC1_IOCTL_GET_VQ_MAX_MSG_SIZE			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 6, uint16_t)

#define ETSOC1_IOCTL_RESET_VQ					\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 7, uint32_t)

#define ETSOC1_IOCTL_PUSH_SQ_MSG				\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 8, cmd_info_t)

#define ETSOC1_IOCTL_POP_CQ_MSG					\
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 9, rsp_info_t)

#define ETSOC1_IOCTL_GET_SQ_AVAILABILITY_BITMAP			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 10, uint32_t)

#define ETSOC1_IOCTL_GET_CQ_AVAILABILITY_BITMAP			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 11, uint32_t)

#define ETSOC1_IOCTL_SET_SQ_AVAILABILITY_THRESHOLD		\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 12, 			\
	sq_availability_threshold_t)

/* TODO: To be removed with Mbox implementation */
#define ETSOC1_IOCTL_GET_DRAM_BASE				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 13, uint64_t)

#define ETSOC1_IOCTL_GET_DRAM_SIZE				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 14, uint64_t)

#define ETSOC1_IOCTL_SET_BULK_CFG				\
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 15, uint32_t)

#define ETSOC1_IOCTL_GET_MBOX_MAX_MSG				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 16, uint64_t)

#define ETSOC1_IOCTL_RESET_MBOX					\
	_IO(ESPERANTO_PCIE_IOCTL_MAGIC, 17)

#define ETSOC1_IOCTL_GET_MBOX_READY				\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 18, uint64_t)

#define ETSOC1_IOCTL_PUSH_MBOX(N)				\
	_IOC(_IOC_WRITE, ESPERANTO_PCIE_IOCTL_MAGIC, 19, (N))

#define ETSOC1_IOCTL_POP_MBOX(N)				\
	_IOC(_IOC_READ, ESPERANTO_PCIE_IOCTL_MAGIC, 20, (N))

#define ETSOC1_IOCTL_GET_FW_UPDATE_REG_BASE			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 21, uint64_t)

#define ETSOC1_IOCTL_GET_FW_UPDATE_REG_SIZE			\
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 22, uint32_t)

#define BULK_CFG_AUTO 0
#define BULK_CFG_MMIO 1
#define BULK_CFG_DMA 2

#endif
