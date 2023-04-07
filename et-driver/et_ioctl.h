#ifndef __ET_IOCTL_H
#define __ET_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESPERANTO_PCIE_IOCTL_MAGIC 0xE7

enum cmd_desc_flag {
	CMD_DESC_FLAG_NONE = 0x0,
	CMD_DESC_FLAG_DMA = 0x1 << 0,
	CMD_DESC_FLAG_MM_RESET = 0x1 << 1,
	CMD_DESC_FLAG_HIGH_PRIORITY = 0x1 << 2,
	CMD_DESC_FLAG_ETSOC_RESET = 0x1 << 3,
	CMD_DESC_FLAG_P2PDMA = 0x1 << 4
};

enum dev_config_form_factor {
	DEV_CONFIG_FORM_FACTOR_NONE = 0,
	DEV_CONFIG_FORM_FACTOR_PCIE,
	DEV_CONFIG_FORM_FACTOR_M_2
};

enum dev_config_arch_revision {
	DEV_CONFIG_ARCH_REVISION_ETSOC1 = 0,
	DEV_CONFIG_ARCH_REVISION_PANTERO,
	DEV_CONFIG_ARCH_REVISION_GEPARDO
};

/*
 * Device State enum
 *
 * DEV_STATE_NOT_READY
 * 	- TODO: SW-10535 To be removed
 * 	- This represents that device is not initialized yet i.e., no device
 * 	  specific structures are initialized. This is the state before probe
 * 	  or when device could never be initialized.
 * DEV_STATE_READY
 * 	- This represents that device is properly initialized and is ready to
 * 	  use.
 * DEV_STATE_RESET_IN_PROGRESS
 * 	- This represents that device is undergoing a reset. User will not be
 * 	  able to open the device node in this state. If the device node is
 * 	  already opened then device node must be closed to let the reset
 * 	  complete. On successful reset, state will be changed to
 * 	  `DEV_STATE_READY` or on failure, state will be changed to
 * 	  `DEV_STATE_NOT_RESPONDING`.
 * DEV_STATE_NOT_RESPONDING
 * 	- This represents that device has undergone a failure. This could be
 * 	  a failure at time of device discovery or during re-initialization
 * 	  of device after reset.
 * DEV_STATE_PENDING_COMMANDS
 * 	- TODO: SW-10535 To be removed
 * 	- This represents the state of Virtual Queues. This is not an internal
 * 	  state of the device. This represents availability of pending command
 * 	  in any submission queue(s) of the device.
 */
enum dev_state {
	DEV_STATE_NOT_READY = 0,
	DEV_STATE_READY,
	DEV_STATE_RESET_IN_PROGRESS,
	DEV_STATE_NOT_RESPONDING,
	DEV_STATE_PENDING_COMMANDS
};

enum trace_buffer_type {
	TRACE_BUFFER_SP = 0,
	TRACE_BUFFER_MM,
	TRACE_BUFFER_CM,
	TRACE_BUFFER_SP_STATS,
	TRACE_BUFFER_MM_STATS,
	TRACE_BUFFER_TYPE_NUM
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

struct dev_config {
	__u32 total_l3_size;		/* in KB */
	__u32 total_l2_size;		/* in KB */
	__u32 total_scp_size;		/* in KB */
	__u32 ddr_bandwidth;		/* in MB/sec */
	__u32 minion_boot_freq;		/* in MHz */
	__u32 cm_shire_mask;		/* Active Compute Shires Mask */
	__u8 form_factor;		/* PCIE or M.2 */
	__u8 tdp;			/* in Watts */
	__u8 cache_line_size;		/* in Bytes */
	__u8 num_l2_cache_banks;	/* Number of L2 Shire Cache banks */
	__u8 sync_min_shire_id;		/* Spare/sync Minion Shire ID */
	__u8 arch_rev;			/* Device architecture revision */
	__u8 devnum;			/* Device physical number */
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

#define ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE                                     \
	_IOW(ESPERANTO_PCIE_IOCTL_MAGIC, 11, __u8)

#define ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER                                      \
	_IOWR(ESPERANTO_PCIE_IOCTL_MAGIC, 12, struct trace_desc)

#define ETSOC1_IOCTL_GET_DEVICE_STATE                                          \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 13, __u32)

#define ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME(N)                                 \
	_IOC(_IOC_READ, ESPERANTO_PCIE_IOCTL_MAGIC, 14, (N))

#define ETSOC1_IOCTL_GET_P2PDMA_DEVICE_COMPAT_BITMAP                           \
	_IOR(ESPERANTO_PCIE_IOCTL_MAGIC, 15, __u64)

#endif
