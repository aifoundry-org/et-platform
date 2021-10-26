#ifndef __ET_IOCTL_H
#define __ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESPERANTO_PCIE_IOCTL_MAGIC 0xE7

// These flags are valid for Ops Device only
enum cmd_desc_flag {
	CMD_DESC_FLAG_NONE = 0,
	CMD_DESC_FLAG_DMA,
	CMD_DESC_FLAG_HIGH_PRIORITY
};

enum dev_config_form_factor {
	DEV_CONFIG_FORM_FACTOR_NONE = 0,
	DEV_CONFIG_FORM_FACTOR_PCIE,
	DEV_CONFIG_FORM_FACTOR_M_2
};

enum dev_state {
	DEV_STATE_READY = 0,
	DEV_STATE_PENDING_COMMANDS,
	DEV_STATE_NOT_RESPONDING
};

// clang-format off

struct fw_update_desc {
	void *ubuf;
	__u64 size;
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
	__u32 dma_max_elem_size;
	__u16 dma_max_elem_count;
	__u16 align_in_bits;
};
//todo update
struct dev_config {
	__u16 form_factor;	/* PCIE or M.2 */
	__u16 tdp;		/* in Watts */
	__u16 total_l3_size;	/* in MB */
	__u16 total_l2_size;	/* in MB */
	__u16 total_scp_size;	/* in MB */
	__u16 cache_line_size;	/* in Bytes */
	__u32 minion_boot_freq;	/* in MHz */
	__u32 cm_shire_mask;	/* Active Compute Shires Mask */
};

struct trace_desc {
	__u8 trace_type;
	void *buf;
};

// clang-format on

#define ETSOC1_IOCTL_GET_USER_DRAM_INFO                                        \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 1, struct dram_info)

#define ETSOC1_IOCTL_FW_UPDATE                                                 \
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 2, struct fw_update_desc)

#define ETSOC1_IOCTL_GET_SQ_COUNT _IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 3, __u16)

#define ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE                                       \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 4, __u16)

#define ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION                                  \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 5, struct dev_config)

#define ETSOC1_IOCTL_PUSH_SQ                                                   \
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 6, struct cmd_desc)

#define ETSOC1_IOCTL_POP_CQ                                                    \
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 7, struct rsp_desc)

#define ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP                                       \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 8, __u64)

#define ETSOC1_IOCTL_GET_CQ_AVAIL_BITMAP                                       \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 9, __u64)

#define ETSOC1_IOCTL_SET_SQ_THRESHOLD                                          \
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 10, struct sq_threshold)

#define ETSOC1_IOCTL_GET_DEVICE_MGMT_TRACE_BUFFER_SIZE                         \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 11, __u32)

#define ETSOC1_IOCTL_EXTRACT_DEVICE_MGMT_TRACE_BUFFER                          \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 12, void *)

#define ETSOC1_IOCTL_GET_DEVICE_STATE                                          \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 13, __u32)

#define ETSOC1_IOCTL_EXTRACT_MM_TRACE_BUFFER                                   \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 14, void *)

#endif
