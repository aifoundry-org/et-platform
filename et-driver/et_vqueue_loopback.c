// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "et_event_handler.h"
#include "et_io.h"
#include "et_pci_dev.h"
#include "et_vqueue.h"

enum mgmt_device_msg_e {
	DM_CMD_GET_MODULE_MANUFACTURE_NAME = 0,
	DM_CMD_GET_MODULE_PART_NUMBER = 1,
	DM_CMD_GET_MODULE_SERIAL_NUMBER = 2,
	DM_CMD_GET_ASIC_CHIP_REVISION = 3,
	DM_CMD_GET_MODULE_DRIVER_REVISION = 4,
	DM_CMD_GET_MODULE_PCIE_ADDR = 5,
	DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED = 6,
	DM_CMD_GET_MODULE_MEMORY_SIZE_MB = 7,
	DM_CMD_GET_MODULE_REVISION = 8,
	DM_CMD_GET_MODULE_FORM_FACTOR = 9,
	DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER = 10,
	DM_CMD_GET_MODULE_MEMORY_TYPE = 11,
	DM_CMD_SET_MODULE_PART_NUMBER = 12,
	DM_CMD_GET_FUSED_PUBLIC_KEYS = 13,
	DM_CMD_GET_MODULE_FIRMWARE_REVISIONS = 14,
	DM_CMD_SET_FIRMWARE_UPDATE = 15,
	DM_CMD_GET_FIRMWARE_BOOT_STATUS = 16,
	DM_CMD_SET_SP_BOOT_ROOT_CERT = 17,
	DM_CMD_SET_SW_BOOT_ROOT_CERT = 18,
	DM_CMD_SET_FIRMWARE_VERSION_COUNTER = 19,
	DM_CMD_SET_FIRMWARE_VALID = 20,
	DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS = 21,
	DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS = 22,
	DM_CMD_GET_MODULE_POWER_STATE = 23,
	DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT = 24,
	DM_CMD_GET_MODULE_STATIC_TDP_LEVEL = 25,
	DM_CMD_SET_MODULE_STATIC_TDP_LEVEL = 26,
	DM_CMD_GET_MODULE_CURRENT_TEMPERATURE = 27,
	DM_CMD_GET_MODULE_TEMPERATURE_THROTTLE_STATUS = 28,
	DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES = 29,
	DM_CMD_GET_MODULE_UPTIME = 30,
	DM_CMD_GET_MODULE_VOLTAGE = 31,
	DM_CMD_GET_ASIC_VOLTAGE = 32,
	DM_CMD_GET_MODULE_POWER = 33,
	DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES = 34,
	DM_CMD_SET_MODULE_VOLTAGE = 35,
	DM_CMD_SET_THROTTLE_POWER_STATE_TEST = 36,
	DM_CMD_SET_FREQUENCY = 37,
	DM_CMD_GET_MODULE_MAX_TEMPERATURE = 38,
	DM_CMD_GET_MODULE_MAX_DDR_BW = 39,
	DM_CMD_GET_MAX_MEMORY_ERROR = 40,
	DM_CMD_SET_DDR_ECC_COUNT = 41,
	DM_CMD_SET_PCIE_ECC_COUNT = 42,
	DM_CMD_SET_SRAM_ECC_COUNT = 43,
	DM_CMD_SET_PCIE_RESET = 44,
	DM_CMD_GET_MODULE_PCIE_ECC_UECC = 45,
	DM_CMD_GET_MODULE_DDR_BW_COUNTER = 46,
	DM_CMD_GET_MODULE_DDR_ECC_UECC = 47,
	DM_CMD_GET_MODULE_SRAM_ECC_UECC = 48,
	DM_CMD_SET_PCIE_MAX_LINK_SPEED = 49,
	DM_CMD_SET_PCIE_LANE_WIDTH = 50,
	DM_CMD_SET_PCIE_RETRAIN_PHY = 51,
	DM_CMD_RESET_ETSOC = 52,
	DM_CMD_GET_ASIC_FREQUENCIES = 53,
	DM_CMD_GET_DRAM_BANDWIDTH = 54,
	DM_CMD_GET_DRAM_CAPACITY_UTILIZATION = 55,
	DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION = 56,
	DM_CMD_GET_ASIC_UTILIZATION = 57,
	DM_CMD_GET_ASIC_STALLS = 58,
	DM_CMD_GET_ASIC_LATENCY = 59,
	DM_CMD_GET_SP_STATS = 60,
	DM_CMD_GET_MM_STATS = 61,
	DM_CMD_SET_STATS_RUN_CONTROL = 62,
	DM_CMD_GET_MM_ERROR_COUNT = 63,
	DM_CMD_MM_RESET = 64,
	DM_CMD_GET_DEVICE_ERROR_EVENTS = 65,
	DM_CMD_SET_DM_TRACE_RUN_CONTROL = 66,
	DM_CMD_SET_DM_TRACE_CONFIG = 67,
};

/*
 * brief Device management command execution status
 */
enum DM_STATUS {
	DM_STATUS_SUCCESS = 0,
	DM_STATUS_ERROR = 1,
};

/*
 * Asset Information
 */
struct asset_info_t {
	char asset[8];
} __packed;

/*
 * Fused Public Keys
 */
struct fused_public_keys_t {
	u8 keys[32];
} __packed;

/*
 * Firmware versions of different minions
 */
struct firmware_version_t {
	u32 bl1_v;
	u32 bl2_v;
	u32 mm_v;
	u32 wm_v;
	u32 machm_v;
	u32 pad;
} __packed;

/*
 * Firmware image path
 */
struct fw_image_path_t {
	char path[64];
} __packed;

/*
 * Fused Public Keys
 */
struct certificate_hash_t {
	char key_blob[48];
	char associated_data[48];
} __packed;

struct temperature_threshold_t {
	u8 temperature;
	u8 pad[7];
} __packed;

struct current_temperature_t {
	u8 temperature_c;
	u8 pad[7];
} __packed;

struct residency_t {
	u64 cumulative;
	u64 average;
	u64 maximum;
	u64 minimum;
} __packed;

struct module_power_t {
	u8 power;
	u8 pad[7];
} __packed;

struct max_temperature_t {
	u8 max_temperature_c;
	u8 pad[7];
} __packed;

struct max_ecc_count_t {
	u32 count;
	u32 pad;
} __packed;

struct ecc_error_count_t {
	u8 count;
	u8 pad[7];

} __packed;

struct errors_count_t {
	u32 ecc;
	u32 uecc;

} __packed;

struct dram_bw_counter_t {
	u32 bw_rd_req_sec;
	u32 bw_wr_req_sec;

} __packed;

struct percentage_cap_t {
	u32 pct_cap;
	u32 pad;

} __packed;

struct mm_error_count_t {
	u32 hang_count;
	u32 exception_count;

} __packed;

struct asic_frequencies {
	u32 minion_shire_mhz;
	u32 noc_mhz;
	u32 mem_shire_mhz;
	u32 ddr_mhz;
	u32 pcie_shire_mhz;
	u32 io_shire_mhz;
} __packed;

struct dram_bw {
	u32 read_req_sec;
	u32 write_req_sec;
} __packed;

struct max_dram_bw {
	u8 max_bw_rd_req_sec;
	u8 max_bw_wr_req_sec;
	u8 pad[6];
} __packed;

struct module_uptime {
	u16 day;
	u8 hours;
	u8 mins;
	u8 pad[4];
} __packed;

struct module_voltage {
	u8 ddr;
	u8 l2_cache;
	u8 maxion;
	u8 minion;
	u8 pcie;
	u8 noc;
	u8 pcie_logic;
	u8 vddqlp;
	u8 vddq;
	u8 pad[7];
} __packed;

struct asic_voltage {
	u16 ddr;
	u16 l2_cache;
	u16 maxion;
	u16 minion;
	u16 pcie;
	u16 noc;
	u16 pcie_logic;
	u16 vddqlp;
	u16 vddq;
	u8 pad[6];
} __packed;

typedef u8 power_state_e;
typedef u8 tdp_level_e;

/*
 * Response header extension for all Device Management responses
 */
struct dev_mgmt_rsp_hdr_extn_t {
	u64 device_latency_usec;
	s32 status;
	u8 pad[4];
} __packed;

/*
 * DM response header. This header is attached to each response
 */
struct dev_mgmt_rsp_header_t {
	struct cmn_header_t rsp_hdr;
	struct dev_mgmt_rsp_hdr_extn_t rsp_hdr_ext;
} __packed;

/*
 * Default response if no specific response is defined for a cmd
 */
struct device_mgmt_default_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	s32 payload;
	u32 pad;
} __packed;

/*
 * Response for the asset info command
 */
struct device_mgmt_asset_tracking_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct asset_info_t asset_info;
} __packed;

/*
 * Response for firmware version command
 */
struct device_mgmt_firmware_versions_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct firmware_version_t firmware_version;
} __packed;

/*
 * Response for temperature threshold update command
 */
struct device_mgmt_temperature_threshold_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct temperature_threshold_t temperature_threshold;
} __packed;

/*
 * Response for power state command
 */
struct device_mgmt_power_state_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	power_state_e pwr_state;
	u8 pad[7];
} __packed;

/*
 * Response for TDL Level command
 */
struct device_mgmt_tdp_level_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	tdp_level_e tdp_level;
	u8 pad[7];
} __packed;

/*
 * Response for current temperature command
 */
struct device_mgmt_current_temperature_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct current_temperature_t current_temperature;
} __packed;

/*
 * Response for throttle residency command
 */
struct device_mgmt_throttle_residency_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct residency_t residency;
} __packed;

/*
 * Response for module power command
 */
struct device_mgmt_module_power_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct module_power_t module_power;
} __packed;

/*
 * Response for module voltage command
 */
struct device_mgmt_get_module_voltage_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct module_voltage module_voltage;
} __packed;

/*
 * Response for get asic voltage command
 */
struct device_mgmt_get_asic_voltage_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct asic_voltage asic_voltage;
} __packed;

/*
 * Response for module uptime command
 */
struct device_mgmt_module_uptime_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct module_uptime module_uptime;
} __packed;

/*
 * Response for max temperature command
 */
struct device_mgmt_max_temperature_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct max_temperature_t max_temperature;
} __packed;

/*
 * Response for max memory error count command
 */
struct device_mgmt_max_memory_error_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct max_ecc_count_t max_ecc_count;
} __packed;

/*
 * Response for Max DRAM BW info command
 */
struct device_mgmt_max_dram_bw_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct max_dram_bw max_dram_bw;
} __packed;

/*
 * Response for power residency command
 */
struct device_mgmt_power_residency_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct residency_t residency;
} __packed;

/*
 * Response for ECC and UECC count command
 */
struct device_mgmt_get_error_count_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct errors_count_t errors_count;
} __packed;

/*
 * Response for DRAM BW info command
 */
struct device_mgmt_dram_bw_counter_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct dram_bw_counter_t dram_bw_counter;
} __packed;

/*
 * Response for asic_frequencies_cmd command
 */
struct device_mgmt_asic_frequencies_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct asic_frequencies asic_frequency;
} __packed;

/*
 * Response for DRAM BW command
 */
struct device_mgmt_dram_bw_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct dram_bw dram_bw; ///
} __packed;

/*
 * Response for DRAM Capacity command
 */
struct device_mgmt_dram_capacity_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct percentage_cap_t percentage_cap; ///
} __packed;

/*
 * Response for ASIC utilization command
 */
struct device_mgmt_asic_per_core_util_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	u64 dummy;
} __packed;

/*
 * Response for ASIC stalls command
 */
struct device_mgmt_asic_stalls_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	u64 dummy;
} __packed;

/*
 * Response for ASIC Latency command
 */
struct device_mgmt_asic_latency_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	u64 dummy;
} __packed;

/*
 * Response for MM state command
 */
struct device_mgmt_mm_state_rsp_t {
	struct dev_mgmt_rsp_header_t rsp_hdr;
	struct mm_error_count_t mm_error_count;
} __packed;

/*
 * Response Header of different response structs in DM services
 */
#define FILL_RSP_HEADER(rsp, tag, msg, latency, sts)                           \
	(rsp).rsp_hdr.rsp_hdr.tag_id = tag;                                    \
	(rsp).rsp_hdr.rsp_hdr.msg_id = msg;                                    \
	(rsp).rsp_hdr.rsp_hdr.size =                                           \
		sizeof(rsp) - sizeof(struct cmn_header_t);                     \
	(rsp).rsp_hdr.rsp_hdr_ext.device_latency_usec = latency;               \
	(rsp).rsp_hdr.rsp_hdr_ext.status = sts

/*
 * Echo command
 */
struct device_ops_echo_cmd_t {
	struct cmd_header_t command_info;
} __packed __aligned(8);

/*
 * Echo response
 */
struct device_ops_echo_rsp_t {
	struct rsp_header_t response_info;
	u64 device_cmd_start_ts; // device timestamp when the command was popped from SQ.
} __packed __aligned(8);

/*
 *  Device compatibility command
 */
struct device_ops_compatibility_cmd_t {
	struct cmd_header_t command_info;
	u16 major;
	u16 minor;
	u16 patch;
	u16 pad;
} __packed __aligned(8);

/*
 * Device compatibility response
 */
struct device_ops_compatibility_rsp_t {
	struct rsp_header_t response_info;
	u16 major;
	u16 minor;
	u16 patch;
	u16 pad;
} __packed __aligned(8);

/*
 * Device firmware version command
 */
struct device_ops_fw_version_cmd_t {
	struct cmd_header_t command_info;
	u8 firmware_type;
	u8 pad[7];
} __packed __aligned(8);

/*
 * Device firmware version response
 */
struct device_ops_fw_version_rsp_t {
	struct rsp_header_t response_info;
	u16 major;
	u16 minor;
	u16 patch;
	u8 type;
	u8 pad;
} __packed __aligned(8);

enum dev_ops_api_kernel_launch_response_e {
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED = 0,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR = 1,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION = 2,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SHIRES_NOT_READY = 3,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED = 4,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS = 5,
	DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG = 6,
};

/*
 * DMA readlist/writelist response status codes
 */
enum dev_ops_api_dma_response_e {
	DEV_OPS_API_DMA_RESPONSE_COMPLETE = 0,
	DEV_OPS_API_DMA_RESPONSE_UNEXPECTED_ERROR = 1,
	DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE = 2,
	DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED = 3,
	DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED = 4,
	DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG = 5,
	DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS = 6,
	DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE = 7,
	DEV_OPS_API_DMA_RESPONSE_CM_IFACE_MULTICAST_FAILED = 8,
	DEV_OPS_API_DMA_RESPONSE_DRIVER_DATA_CONFIG_FAILED = 9,
	DEV_OPS_API_DMA_RESPONSE_DRIVER_LINK_CONFIG_FAILED = 10,
	DEV_OPS_API_DMA_RESPONSE_DRIVER_CHAN_START_FAILED = 11,
	DEV_OPS_API_DMA_RESPONSE_DRIVER_ABORT_FAILED = 12
};

/*
 * Launch a kernel on the target
 */
struct device_ops_kernel_launch_cmd_t {
	struct cmd_header_t command_info;
	u64 code_start_address;
	u64 pointer_to_args;
	u64 shire_mask;
} __packed __aligned(8);

/*
 * Response and result of a kernel launch on the device
 */
struct device_ops_kernel_launch_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_start_ts;
	u64 cmd_execution_time;
	u64 cmd_wait_time;
	u32 status;
	u8 pad[4];
} __packed __aligned(8);

enum dev_ops_api_kernel_abort_response_e {
	DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS = 0,
	DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR = 1,
	DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID = 2,
	DEV_OPS_API_KERNEL_ABORT_RESPONSE_TIMEOUT_HANG = 3,
};

/*
 * Command to abort a currently running kernel on the device
 */
struct device_ops_kernel_abort_cmd_t {
	struct cmd_header_t command_info;
	u16 kernel_launch_tag_id;
	u8 pad[6];
} __packed __aligned(8);

/*
 * Response to an abort request
 */
struct device_ops_kernel_abort_rsp_t {
	struct rsp_header_t response_info;
	u32 status;
	u32 pad;
} __packed __aligned(8);

static struct et_msg_node *create_msg_node(u32 msg_size)
{
	struct et_msg_node *new_node;
	//Build node
	new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);

	if (IS_ERR(new_node)) {
		panic("Failed to allocate msg node, error %ld\n",
		      PTR_ERR(new_node));
		return NULL;
	}

	new_node->msg = kmalloc(msg_size, GFP_KERNEL);

	if (IS_ERR(new_node->msg)) {
		panic("Failed to allocate msg buffer, error %ld\n",
		      PTR_ERR(new_node->msg));
		return NULL;
	}

	new_node->msg_size = msg_size;

	return new_node;
}

static void enqueue_msg_node(struct et_cqueue *cq, struct et_msg_node *msg)
{
	mutex_lock(&cq->msg_list_mutex);
	list_add_tail(&msg->list, &cq->msg_list);
	mutex_unlock(&cq->msg_list_mutex);
}

struct et_msg_node *et_dequeue_msg_node(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node, list);
	if (msg)
		list_del(&msg->list);

	mutex_unlock(&cq->msg_list_mutex);

	return msg;
}

static void destroy_msg_node(struct et_msg_node *node)
{
	if (node) {
		kfree(node->msg);
		kfree(node);
	}
}

static void mm_reset_completion_callback(struct et_cqueue *cq,
					 struct device_mgmt_rsp_hdr_t *rsp)
{
	int rv = 0;
	struct et_pci_dev *et_dev = pci_get_drvdata(cq->vq_common->pdev);

	if (rsp->status != DEV_OPS_API_MM_RESET_RESPONSE_COMPLETE) {
		dev_err(&et_dev->pdev->dev,
			"MM reset failed!, status: %d\n",
			rsp->status);
		return;
	}

	et_ops_dev_destroy(et_dev, false);
	rv = et_ops_dev_init(et_dev, 0, false);
	if (rv)
		dev_err(&et_dev->pdev->dev,
			"Ops device re-initialization failed, errno: %d\n",
			-rv);
}

void et_destroy_msg_list(struct et_cqueue *cq)
{
	struct list_head *pos, *next;
	struct et_msg_node *node;
	int count = 0;

	mutex_lock(&cq->msg_list_mutex);
	list_for_each_safe (pos, next, &cq->msg_list) {
		node = list_entry(pos, struct et_msg_node, list);
		list_del(pos);
		destroy_msg_node(node);
		count++;
	}
	mutex_unlock(&cq->msg_list_mutex);

	if (count)
		pr_warn("Discarded (%d) CQ user messages", count);
}

bool et_cqueue_msg_available(struct et_cqueue *cq)
{
	struct et_msg_node *msg;

	mutex_lock(&cq->msg_list_mutex);
	msg = list_first_entry_or_null(&cq->msg_list, struct et_msg_node, list);
	mutex_unlock(&cq->msg_list_mutex);

	return !!(msg);
}

void et_squeue_sync_cb_for_host(struct et_squeue *sq);

static void et_sq_isr_work(struct work_struct *work)
{
	struct et_squeue *sq = container_of(work, struct et_squeue, isr_work);

	// Check for SQ availability and wake up the waitqueue if available
	if (test_bit(sq->index, sq->vq_common->sq_bitmap))
		goto update_sq_bitmap;

	et_squeue_sync_cb_for_host(sq);

update_sq_bitmap:
	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold)) {
		set_bit(sq->index, sq->vq_common->sq_bitmap);
		wake_up_interruptible(&sq->vq_common->waitqueue);
	}

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);
}

static void et_cq_isr_work(struct work_struct *work)
{
	struct et_cqueue *cq = container_of(work, struct et_cqueue, isr_work);

	et_cqueue_isr_bottom(cq);
}

static ssize_t et_high_priority_squeue_init_all(struct et_pci_dev *et_dev,
						bool is_mgmt)
{
	ssize_t i;
	struct et_mapped_region *vq_region;
	u8 __iomem *hp_sq_baseaddr;
	struct et_vq_data *vq_data;
	u16 hp_sq_size;

	if (is_mgmt) {
		dev_dbg(&et_dev->pdev->dev, "Mgmt: HP SQs are not supported\n");
		return 0;
	}

	if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER].is_valid) {
		return -EINVAL;
	}

	vq_data = &et_dev->ops.vq_data;
	vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
	hp_sq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
			 et_dev->ops.dir_vq.hp_sq_offset;
	hp_sq_size = et_dev->ops.dir_vq.hp_sq_size;

	vq_data->hp_sqs = kmalloc_array(vq_data->vq_common.hp_sq_count,
					sizeof(*vq_data->hp_sqs),
					GFP_KERNEL);
	if (!vq_data->hp_sqs)
		return -ENOMEM;

	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		vq_data->hp_sqs[i].index = i;
		vq_data->hp_sqs[i].is_hp_sq = true;
		vq_data->hp_sqs[i].vq_common = &vq_data->vq_common;
		vq_data->hp_sqs[i].cb_mem =
			(struct et_circbuffer __iomem *)hp_sq_baseaddr;
		vq_data->hp_sqs[i].cb.head = 0;
		vq_data->hp_sqs[i].cb.tail = 0;
		vq_data->hp_sqs[i].cb.len =
			hp_sq_size - sizeof(struct et_circbuffer);
		et_iowrite(vq_data->hp_sqs[i].cb_mem,
			   0,
			   (u8 *)&vq_data->hp_sqs[i].cb,
			   sizeof(vq_data->hp_sqs[i].cb));
		hp_sq_baseaddr += hp_sq_size;

		mutex_init(&vq_data->hp_sqs[i].push_mutex);
		et_vq_stats_init(&vq_data->hp_sqs[i].stats);
	}

	return 0;
}

static ssize_t et_squeue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i, rv;
	struct et_mapped_region *vq_region;
	struct et_vq_data *vq_data;
	u8 __iomem *sq_baseaddr;
	u16 sq_size;

	if (is_mgmt) {
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->mgmt.vq_data;
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
		sq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
			      et_dev->mgmt.dir_vq.sq_offset;
		sq_size = et_dev->mgmt.dir_vq.sq_size;
	} else {
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->ops.vq_data;
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
		sq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
			      et_dev->ops.dir_vq.sq_offset;
		sq_size = et_dev->ops.dir_vq.sq_size;
	}

	// Initialize sq_workqueue
	vq_data->vq_common.sq_workqueue =
		alloc_workqueue("%s:%s%d_sqwq",
				WQ_MEM_RECLAIM | WQ_UNBOUND,
				vq_data->vq_common.sq_count,
				dev_name(&et_dev->pdev->dev),
				(is_mgmt) ? "mgmt" : "ops",
				et_dev->dev_index);
	if (!vq_data->vq_common.sq_workqueue)
		return -ENOMEM;

	vq_data->sqs = kmalloc_array(vq_data->vq_common.sq_count,
				     sizeof(*vq_data->sqs),
				     GFP_KERNEL);
	if (!vq_data->sqs) {
		rv = -ENOMEM;
		goto error_destroy_sq_workqueue;
	}

	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		vq_data->sqs[i].index = i;
		vq_data->sqs[i].is_hp_sq = false;
		vq_data->sqs[i].vq_common = &vq_data->vq_common;
		vq_data->sqs[i].cb_mem =
			(struct et_circbuffer __iomem *)sq_baseaddr;
		vq_data->sqs[i].cb.head = 0;
		vq_data->sqs[i].cb.tail = 0;
		vq_data->sqs[i].cb.len = sq_size - sizeof(struct et_circbuffer);
		et_iowrite(vq_data->sqs[i].cb_mem,
			   0,
			   (u8 *)&vq_data->sqs[i].cb,
			   sizeof(vq_data->sqs[i].cb));
		sq_baseaddr += sq_size;

		mutex_init(&vq_data->sqs[i].push_mutex);
		atomic_set(&vq_data->sqs[i].sq_threshold,
			   (sq_size - sizeof(struct et_circbuffer)) / 4);

		// Init statistics before work handler
		et_vq_stats_init(&vq_data->sqs[i].stats);

		INIT_WORK(&vq_data->sqs[i].isr_work, et_sq_isr_work);
		queue_work(vq_data->vq_common.sq_workqueue,
			   &vq_data->sqs[i].isr_work);
		flush_workqueue(vq_data->vq_common.sq_workqueue);
	}

	return 0;

error_destroy_sq_workqueue:
	destroy_workqueue(vq_data->vq_common.sq_workqueue);

	return rv;
}

static ssize_t et_cqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i, rv;
	struct et_mapped_region *vq_region;
	struct et_vq_data *vq_data;
	u8 __iomem *cq_baseaddr;
	u16 cq_size;

	if (is_mgmt) {
		if (!et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->mgmt.vq_data;
		vq_region =
			&et_dev->mgmt.regions[MGMT_MEM_REGION_TYPE_VQ_BUFFER];
		cq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
			      et_dev->mgmt.dir_vq.cq_offset;
		cq_size = et_dev->mgmt.dir_vq.cq_size;
	} else {
		if (!et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER]
			     .is_valid) {
			return -EINVAL;
		}
		vq_data = &et_dev->ops.vq_data;
		vq_region = &et_dev->ops.regions[OPS_MEM_REGION_TYPE_VQ_BUFFER];
		cq_baseaddr = (u8 __iomem *)vq_region->mapped_baseaddr +
			      et_dev->ops.dir_vq.cq_offset;
		cq_size = et_dev->ops.dir_vq.cq_size;
	}

	// Initialize cq_workqueue
	vq_data->vq_common.cq_workqueue =
		alloc_workqueue("%s:%s%d_cqwq",
				WQ_MEM_RECLAIM | WQ_UNBOUND,
				vq_data->vq_common.cq_count,
				dev_name(&et_dev->pdev->dev),
				(is_mgmt) ? "mgmt" : "ops",
				et_dev->dev_index);
	if (!vq_data->vq_common.cq_workqueue)
		return -ENOMEM;

	vq_data->cqs = kmalloc_array(vq_data->vq_common.cq_count,
				     sizeof(*vq_data->cqs),
				     GFP_KERNEL);
	if (!vq_data->cqs) {
		rv = -ENOMEM;
		goto error_destroy_cq_workqueue;
	}

	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		vq_data->cqs[i].index = i;
		vq_data->cqs[i].vq_common = &vq_data->vq_common;
		vq_data->cqs[i].cb_mem =
			(struct et_circbuffer __iomem *)cq_baseaddr;
		vq_data->cqs[i].cb.head = 0;
		vq_data->cqs[i].cb.tail = 0;
		vq_data->cqs[i].cb.len = cq_size - sizeof(struct et_circbuffer);
		et_iowrite(vq_data->cqs[i].cb_mem,
			   0,
			   (u8 *)&vq_data->cqs[i].cb,
			   sizeof(vq_data->cqs[i].cb));
		cq_baseaddr += cq_size;

		mutex_init(&vq_data->cqs[i].pop_mutex);
		INIT_LIST_HEAD(&vq_data->cqs[i].msg_list);
		mutex_init(&vq_data->cqs[i].msg_list_mutex);

		// Init statistics before work handler
		et_vq_stats_init(&vq_data->cqs[i].stats);

		INIT_WORK(&vq_data->cqs[i].isr_work, et_cq_isr_work);
		queue_work(vq_data->vq_common.cq_workqueue,
			   &vq_data->cqs[i].isr_work);
		flush_workqueue(vq_data->vq_common.cq_workqueue);
	}

	vq_data->vq_common.intrpt_addr = (void __iomem __force *)vq_data->cqs;

	return 0;

error_destroy_cq_workqueue:
	destroy_workqueue(vq_data->vq_common.cq_workqueue);

	return rv;
}

static void et_high_priority_squeue_destroy_all(struct et_pci_dev *et_dev,
						bool is_mgmt);
static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt);

ssize_t et_vqueue_init_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t rv;
	struct et_vq_common *vq_common;

	if (is_mgmt) {
		vq_common = &et_dev->mgmt.vq_data.vq_common;
		vq_common->sq_count = et_dev->mgmt.dir_vq.sq_count;
		vq_common->sq_size = et_dev->mgmt.dir_vq.sq_size;
		vq_common->hp_sq_count = 0;
		vq_common->hp_sq_size = 0;
		vq_common->cq_count = et_dev->mgmt.dir_vq.cq_count;
		vq_common->cq_size = et_dev->mgmt.dir_vq.cq_size;
	} else {
		vq_common = &et_dev->ops.vq_data.vq_common;
		vq_common->sq_count = et_dev->ops.dir_vq.sq_count;
		vq_common->sq_size = et_dev->ops.dir_vq.sq_size;
		vq_common->hp_sq_count = et_dev->ops.dir_vq.hp_sq_count;
		vq_common->hp_sq_size = et_dev->ops.dir_vq.hp_sq_size;
		vq_common->cq_count = et_dev->ops.dir_vq.cq_count;
		vq_common->cq_size = et_dev->ops.dir_vq.cq_size;
	}

	bitmap_zero(vq_common->sq_bitmap, ET_MAX_QUEUES);
	mutex_init(&vq_common->sq_bitmap_mutex);
	bitmap_zero(vq_common->cq_bitmap, ET_MAX_QUEUES);
	mutex_init(&vq_common->cq_bitmap_mutex);
	init_waitqueue_head(&vq_common->waitqueue);
	vq_common->pdev = et_dev->pdev;

	rv = et_high_priority_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		return rv;

	rv = et_squeue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_high_priority_squeue_destroy_all;

	rv = et_cqueue_init_all(et_dev, is_mgmt);
	if (rv)
		goto error_squeue_destroy_all;

	return rv;

error_squeue_destroy_all:
	et_squeue_destroy_all(et_dev, is_mgmt);

error_high_priority_squeue_destroy_all:
	et_high_priority_squeue_destroy_all(et_dev, is_mgmt);

	return rv;
}

static void et_high_priority_squeue_destroy_all(struct et_pci_dev *et_dev,
						bool is_mgmt)
{
	ssize_t i;
	struct et_vq_data *vq_data;

	if (is_mgmt)
		return;

	vq_data = &et_dev->ops.vq_data;
	for (i = 0; i < vq_data->vq_common.hp_sq_count; i++) {
		mutex_destroy(&vq_data->hp_sqs[i].push_mutex);
		vq_data->hp_sqs[i].cb_mem = NULL;
		vq_data->hp_sqs[i].vq_common = NULL;
	}

	kfree(vq_data->hp_sqs);
}

static void et_squeue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	ssize_t i;
	struct et_vq_data *vq_data;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	for (i = 0; i < vq_data->vq_common.sq_count; i++) {
		cancel_work_sync(&vq_data->sqs[i].isr_work);
		mutex_destroy(&vq_data->sqs[i].push_mutex);
		vq_data->sqs[i].cb_mem = NULL;
		vq_data->sqs[i].vq_common = NULL;
	}

	kfree(vq_data->sqs);
	destroy_workqueue(vq_data->vq_common.sq_workqueue);
}

static void et_cqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	int i;
	struct et_vq_data *vq_data;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	for (i = 0; i < vq_data->vq_common.cq_count; i++) {
		cancel_work_sync(&vq_data->cqs[i].isr_work);
		mutex_destroy(&vq_data->cqs[i].pop_mutex);
		et_destroy_msg_list(&vq_data->cqs[i]);
		mutex_destroy(&vq_data->cqs[i].msg_list_mutex);
		vq_data->cqs[i].cb_mem = NULL;
		vq_data->cqs[i].vq_common = NULL;
	}

	kfree(vq_data->cqs);
	destroy_workqueue(vq_data->vq_common.cq_workqueue);
}

void et_vqueue_destroy_all(struct et_pci_dev *et_dev, bool is_mgmt)
{
	struct et_vq_data *vq_data;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	wake_up_all(&vq_data->vq_common.waitqueue);
	while (waitqueue_active(&vq_data->vq_common.waitqueue))
		msleep(1);

	et_cqueue_destroy_all(et_dev, is_mgmt);
	et_squeue_destroy_all(et_dev, is_mgmt);
	et_high_priority_squeue_destroy_all(et_dev, is_mgmt);

	mutex_destroy(&vq_data->vq_common.sq_bitmap_mutex);
	mutex_destroy(&vq_data->vq_common.cq_bitmap_mutex);
}

static inline void loopback_interrupt(struct et_squeue *sq,
				      struct et_cqueue *cq)
{
	queue_work(sq->vq_common->sq_workqueue, &sq->isr_work);
	queue_work(cq->vq_common->cq_workqueue, &cq->isr_work);
}

static ssize_t cmd_loopback_handler(struct et_squeue *sq)
{
	u8 *cmd;
	ssize_t rv = 0;
	struct cmn_header_t header;
	struct device_mgmt_default_rsp_t dm_def_rsp;
	struct device_mgmt_get_error_count_rsp_t dm_gec_rsp;
	struct device_mgmt_dram_bw_counter_rsp_t dm_dbc_rsp;
	struct device_mgmt_max_memory_error_rsp_t dm_mme_rsp;
	struct device_mgmt_max_dram_bw_rsp_t dm_mdb_rsp;
	struct device_mgmt_power_residency_rsp_t dm_mtt_rsp;
	struct device_mgmt_max_temperature_rsp_t dm_mt_rsp;
	struct device_mgmt_firmware_versions_rsp_t dm_fv_rsp;
	struct device_mgmt_asset_tracking_rsp_t dm_at_rsp;
	struct device_mgmt_power_state_rsp_t dm_ps_rsp;
	struct device_mgmt_tdp_level_rsp_t dm_tl_rsp;
	struct device_mgmt_temperature_threshold_rsp_t dm_tt_rsp;
	struct device_mgmt_current_temperature_rsp_t dm_ct_rsp;
	struct device_mgmt_throttle_residency_rsp_t dm_tht_rsp;
	struct device_mgmt_module_power_rsp_t dm_mp_rsp;
	struct device_mgmt_get_module_voltage_rsp_t dm_mv_rsp;
	struct device_mgmt_get_asic_voltage_rsp_t dm_av_rsp;
	struct device_mgmt_module_uptime_rsp_t dm_mu_rsp;
	struct device_mgmt_asic_frequencies_rsp_t dm_af_rsp;
	struct device_mgmt_dram_bw_rsp_t dm_db_rsp;
	struct device_mgmt_dram_capacity_rsp_t dm_dc_rsp;
	struct device_mgmt_asic_per_core_util_rsp_t dm_apcu_rsp;
	struct device_mgmt_asic_stalls_rsp_t dm_as_rsp;
	struct device_mgmt_asic_latency_rsp_t dm_al_rsp;
	struct device_mgmt_mm_state_rsp_t dm_ms_rsp;
	struct device_ops_echo_cmd_t *echo_cmd;
	struct device_ops_echo_rsp_t echo_rsp;
	struct device_ops_compatibility_cmd_t *compat_cmd;
	struct device_ops_compatibility_rsp_t compat_rsp;
	struct device_ops_fw_version_cmd_t *fw_version_cmd;
	struct device_ops_fw_version_rsp_t fw_version_rsp;
	struct device_ops_dma_readlist_cmd_t *dma_readlist_cmd;
	struct device_ops_dma_readlist_rsp_t dma_readlist_rsp;
	struct device_ops_dma_writelist_cmd_t *dma_writelist_cmd;
	struct device_ops_dma_writelist_rsp_t dma_writelist_rsp;
	struct device_ops_kernel_launch_cmd_t *kernel_launch_cmd;
	struct device_ops_kernel_launch_rsp_t kernel_launch_rsp;
	struct device_ops_kernel_abort_cmd_t *kernel_abort_cmd;
	struct device_ops_kernel_abort_rsp_t kernel_abort_rsp;
	struct et_cqueue *cqs =
		(struct et_cqueue __force *)sq->vq_common->intrpt_addr;
	struct et_cqueue *cq = &cqs[0];

	// Read the message header
	if (!et_circbuffer_pop(&sq->cb,
			       sq->cb_mem,
			       (u8 *)&header,
			       sizeof(header),
			       ET_CB_SYNC_FOR_HOST))
		return -EAGAIN;

	cmd = kzalloc(header.size, GFP_KERNEL);
	memcpy(cmd, (u8 *)&header, sizeof(header));

	// Read the message payload
	if (!et_circbuffer_pop(&sq->cb,
			       sq->cb_mem,
			       cmd + sizeof(header),
			       header.size - sizeof(header),
			       ET_CB_SYNC_FOR_DEVICE)) {
		rv = -EAGAIN;
		goto error_free_cmd_mem;
	}

	mutex_lock(&cq->pop_mutex);
	switch (header.msg_id) {
	case DEV_OPS_API_MID_ECHO_CMD:
		echo_cmd = (struct device_ops_echo_cmd_t *)cmd;
		echo_rsp.response_info.rsp_hdr.size =
			sizeof(echo_rsp) - sizeof(header);
		echo_rsp.response_info.rsp_hdr.tag_id =
			echo_cmd->command_info.cmd_hdr.tag_id;
		echo_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_ECHO_RSP;
		// send dummy timestamp
		echo_rsp.device_cmd_start_ts = 0xdeadbeef;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&echo_rsp,
					sizeof(echo_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DEV_OPS_API_MID_COMPATIBILITY_CMD:
		compat_cmd = (struct device_ops_compatibility_cmd_t *)cmd;
		compat_rsp.response_info.rsp_hdr.size =
			sizeof(compat_rsp) - sizeof(header);
		compat_rsp.response_info.rsp_hdr.tag_id =
			compat_cmd->command_info.cmd_hdr.tag_id;
		compat_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_COMPATIBILITY_RSP;
		compat_rsp.major = 0;
		compat_rsp.minor = 1;
		compat_rsp.patch = 0;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&compat_rsp,
					sizeof(compat_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DEV_OPS_API_MID_FW_VERSION_CMD:
		fw_version_cmd = (struct device_ops_fw_version_cmd_t *)cmd;
		fw_version_rsp.response_info.rsp_hdr.size =
			sizeof(fw_version_rsp) - sizeof(header);
		fw_version_rsp.response_info.rsp_hdr.tag_id =
			fw_version_cmd->command_info.cmd_hdr.tag_id;
		fw_version_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_FW_VERSION_RSP;
		fw_version_rsp.major = 0;
		fw_version_rsp.minor = 0;
		fw_version_rsp.patch = 0;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&fw_version_rsp,
					sizeof(fw_version_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DEV_OPS_API_MID_DMA_READLIST_CMD:
		dma_readlist_cmd = (struct device_ops_dma_readlist_cmd_t *)cmd;
		dma_readlist_rsp.response_info.rsp_hdr.size =
			sizeof(dma_readlist_rsp) - sizeof(header);
		dma_readlist_rsp.response_info.rsp_hdr.tag_id =
			dma_readlist_cmd->command_info.cmd_hdr.tag_id;
		dma_readlist_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_DMA_READLIST_RSP;
		dma_readlist_rsp.status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dma_readlist_rsp,
					sizeof(dma_readlist_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DEV_OPS_API_MID_DMA_WRITELIST_CMD:
		dma_writelist_cmd =
			(struct device_ops_dma_writelist_cmd_t *)cmd;
		dma_writelist_rsp.response_info.rsp_hdr.size =
			sizeof(dma_writelist_rsp) - sizeof(header);
		dma_writelist_rsp.response_info.rsp_hdr.tag_id =
			dma_writelist_cmd->command_info.cmd_hdr.tag_id;
		dma_writelist_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_DMA_WRITELIST_RSP;
		dma_writelist_rsp.status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dma_writelist_rsp,
					sizeof(dma_writelist_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;
	case DEV_OPS_API_MID_KERNEL_LAUNCH_CMD:
		kernel_launch_cmd =
			(struct device_ops_kernel_launch_cmd_t *)cmd;
		kernel_launch_rsp.response_info.rsp_hdr.size =
			sizeof(kernel_launch_rsp) - sizeof(header);
		kernel_launch_rsp.response_info.rsp_hdr.tag_id =
			kernel_launch_cmd->command_info.cmd_hdr.tag_id;
		kernel_launch_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_KERNEL_LAUNCH_RSP;
		kernel_launch_rsp.status =
			DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&kernel_launch_rsp,
					sizeof(kernel_launch_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DEV_OPS_API_MID_KERNEL_ABORT_CMD:
		kernel_abort_cmd = (struct device_ops_kernel_abort_cmd_t *)cmd;
		kernel_abort_rsp.response_info.rsp_hdr.size =
			sizeof(kernel_abort_rsp) - sizeof(header);
		kernel_abort_rsp.response_info.rsp_hdr.tag_id =
			kernel_abort_cmd->command_info.cmd_hdr.tag_id;
		kernel_abort_rsp.response_info.rsp_hdr.msg_id =
			DEV_OPS_API_MID_KERNEL_ABORT_RSP;
		kernel_abort_rsp.status =
			DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS;
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&kernel_abort_rsp,
					sizeof(kernel_abort_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_PCIE_RESET:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_PCIE_RESET,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_PCIE_MAX_LINK_SPEED:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_PCIE_MAX_LINK_SPEED,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_PCIE_LANE_WIDTH:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_PCIE_LANE_WIDTH,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_PCIE_RETRAIN_PHY:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_PCIE_RETRAIN_PHY,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_PCIE_ECC_UECC:
		FILL_RSP_HEADER(dm_gec_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_PCIE_ECC_UECC,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_gec_rsp,
					sizeof(dm_gec_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_DDR_ECC_UECC:
		FILL_RSP_HEADER(dm_gec_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_DDR_ECC_UECC,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_gec_rsp,
					sizeof(dm_gec_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_SRAM_ECC_UECC:
		FILL_RSP_HEADER(dm_gec_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_SRAM_ECC_UECC,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_gec_rsp,
					sizeof(dm_gec_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_DDR_BW_COUNTER:
		FILL_RSP_HEADER(dm_dbc_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_DDR_BW_COUNTER,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_dbc_rsp,
					sizeof(dm_dbc_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_DDR_ECC_COUNT:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_DDR_ECC_COUNT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_PCIE_ECC_COUNT:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_PCIE_ECC_COUNT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_SRAM_ECC_COUNT:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_SRAM_ECC_COUNT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MAX_MEMORY_ERROR:
		FILL_RSP_HEADER(dm_mme_rsp,
				header.tag_id,
				DM_CMD_GET_MAX_MEMORY_ERROR,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mme_rsp,
					sizeof(dm_mme_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_MAX_DDR_BW:
		FILL_RSP_HEADER(dm_mdb_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MAX_DDR_BW,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mdb_rsp,
					sizeof(dm_mdb_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_MAX_TEMPERATURE:
		FILL_RSP_HEADER(dm_mt_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MAX_TEMPERATURE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mt_rsp,
					sizeof(dm_mt_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_FIRMWARE_UPDATE:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_FIRMWARE_UPDATE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_FIRMWARE_REVISIONS:
		FILL_RSP_HEADER(dm_fv_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_FIRMWARE_REVISIONS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_fv_rsp,
					sizeof(dm_fv_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_FIRMWARE_BOOT_STATUS:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_GET_FIRMWARE_BOOT_STATUS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_SP_BOOT_ROOT_CERT:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_SP_BOOT_ROOT_CERT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_RESET_ETSOC:
		// Do nothing, no response will be returned
		break;

	case DM_CMD_GET_MODULE_MANUFACTURE_NAME:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MANUFACTURE_NAME,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "Esperan");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_PART_NUMBER:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_PART_NUMBER,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "ETPART1");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_SERIAL_NUMBER:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_SERIAL_NUMBER,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "ETSER_1");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_CHIP_REVISION:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_CHIP_REVISION,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_REVISION:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_REVISION,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%d", 1);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_FORM_FACTOR:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_FORM_FACTOR,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "Dual_M2");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "Unknown");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_MEMORY_SIZE_MB:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MEMORY_SIZE_MB,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%d", 16 * 1024);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_MEMORY_TYPE:
		FILL_RSP_HEADER(dm_at_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_MEMORY_TYPE,
				0,
				DM_STATUS_SUCCESS);
		sprintf(dm_at_rsp.asset_info.asset, "%s", "LPDDR4X");
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_at_rsp,
					sizeof(dm_at_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_POWER_STATE:
		FILL_RSP_HEADER(dm_ps_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_POWER_STATE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_ps_rsp,
					sizeof(dm_ps_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_STATIC_TDP_LEVEL:
		FILL_RSP_HEADER(dm_tl_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_STATIC_TDP_LEVEL,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_tl_rsp,
					sizeof(dm_tl_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_MODULE_STATIC_TDP_LEVEL:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_MODULE_STATIC_TDP_LEVEL,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS:
		FILL_RSP_HEADER(dm_tt_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_tt_rsp,
					sizeof(dm_tt_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_CURRENT_TEMPERATURE:
		FILL_RSP_HEADER(dm_ct_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_CURRENT_TEMPERATURE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_ct_rsp,
					sizeof(dm_ct_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES:
		FILL_RSP_HEADER(dm_tht_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_tht_rsp,
					sizeof(dm_tht_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES:
		FILL_RSP_HEADER(dm_mtt_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mtt_rsp,
					sizeof(dm_mtt_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_POWER:
		FILL_RSP_HEADER(dm_mp_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_POWER,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mp_rsp,
					sizeof(dm_mp_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_VOLTAGE:
		FILL_RSP_HEADER(dm_mv_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_VOLTAGE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mv_rsp,
					sizeof(dm_mv_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_VOLTAGE:
		FILL_RSP_HEADER(dm_av_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_VOLTAGE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_av_rsp,
					sizeof(dm_av_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MODULE_UPTIME:
		FILL_RSP_HEADER(dm_mu_rsp,
				header.tag_id,
				DM_CMD_GET_MODULE_UPTIME,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_mu_rsp,
					sizeof(dm_mu_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_FREQUENCIES:
		FILL_RSP_HEADER(dm_af_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_FREQUENCIES,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_af_rsp,
					sizeof(dm_af_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_DRAM_BANDWIDTH:
		FILL_RSP_HEADER(dm_db_rsp,
				header.tag_id,
				DM_CMD_GET_DRAM_BANDWIDTH,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_db_rsp,
					sizeof(dm_db_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_DRAM_CAPACITY_UTILIZATION:
		FILL_RSP_HEADER(dm_dc_rsp,
				header.tag_id,
				DM_CMD_GET_DRAM_CAPACITY_UTILIZATION,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_dc_rsp,
					sizeof(dm_dc_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION:
		FILL_RSP_HEADER(dm_apcu_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_apcu_rsp,
					sizeof(dm_apcu_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_UTILIZATION:
		FILL_RSP_HEADER(dm_apcu_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_UTILIZATION,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_apcu_rsp,
					sizeof(dm_apcu_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_STALLS:
		FILL_RSP_HEADER(dm_as_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_STALLS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_as_rsp,
					sizeof(dm_as_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_ASIC_LATENCY:
		FILL_RSP_HEADER(dm_al_rsp,
				header.tag_id,
				DM_CMD_GET_ASIC_LATENCY,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_al_rsp,
					sizeof(dm_al_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_MM_ERROR_COUNT:
		FILL_RSP_HEADER(dm_ms_rsp,
				header.tag_id,
				DM_CMD_GET_MM_ERROR_COUNT,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_ms_rsp,
					sizeof(dm_ms_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_MM_RESET:
		FILL_RSP_HEADER(dm_ms_rsp,
				header.tag_id,
				DM_CMD_MM_RESET,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_ms_rsp,
					sizeof(dm_ms_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_GET_DEVICE_ERROR_EVENTS:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_GET_DEVICE_ERROR_EVENTS,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_DM_TRACE_RUN_CONTROL:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_DM_TRACE_RUN_CONTROL,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_DM_TRACE_CONFIG:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_DM_TRACE_CONFIG,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_MODULE_VOLTAGE:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_MODULE_VOLTAGE,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_THROTTLE_POWER_STATE_TEST:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_THROTTLE_POWER_STATE_TEST,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;

	case DM_CMD_SET_FREQUENCY:
		FILL_RSP_HEADER(dm_def_rsp,
				header.tag_id,
				DM_CMD_SET_FREQUENCY,
				0,
				DM_STATUS_SUCCESS);
		if (!et_circbuffer_push(&cq->cb,
					cq->cb_mem,
					(u8 *)&dm_def_rsp,
					sizeof(dm_def_rsp),
					ET_CB_SYNC_FOR_HOST |
						ET_CB_SYNC_FOR_DEVICE))
			rv = -EAGAIN;
		break;
	}
	mutex_unlock(&cq->pop_mutex);

	loopback_interrupt(sq, cq);

error_free_cmd_mem:
	kfree(cmd);

	return rv;
}

ssize_t et_squeue_push(struct et_squeue *sq, void *buf, size_t count)
{
	struct cmn_header_t *header = buf;
	ssize_t rv;

	if (count < sizeof(*header)) {
		pr_err("VQ[%d]: size too small: %ld", sq->index, count);
		return -EINVAL;
	}

	if (header->size > count) {
		pr_err("VQ[%d]: header contains invalid cmd size", sq->index);
		return -EINVAL;
	}

	mutex_lock(&sq->push_mutex);

	if (!et_circbuffer_push(&sq->cb,
				sq->cb_mem,
				buf,
				header->size,
				ET_CB_SYNC_FOR_HOST | ET_CB_SYNC_FOR_DEVICE)) {
		// Full; no room for message, returning EAGAIN
		rv = -EAGAIN;
		goto update_sq_bitmap;
	}

	atomic64_inc(&sq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
	et_rate_entry_update(1, &sq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
	atomic64_add(header->size,
		     &sq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
	et_rate_entry_update(header->size,
			     &sq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

	rv = cmd_loopback_handler(sq);
	if (rv) {
		// cmd_loopback_handler couldn't push response to CQ
		goto update_sq_bitmap;
	}

	rv = count;

update_sq_bitmap:
	mutex_unlock(&sq->push_mutex);

	if (sq->is_hp_sq)
		return rv;

	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) < atomic_read(&sq->sq_threshold)) {
		clear_bit(sq->index, sq->vq_common->sq_bitmap);
		wake_up_interruptible(&sq->vq_common->waitqueue);
	}

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);

	return rv;
}

ssize_t et_squeue_copy_from_user(struct et_pci_dev *et_dev,
				 bool is_mgmt,
				 bool is_hp_sq,
				 u16 sq_index,
				 const char __user *ubuf,
				 size_t count)
{
	struct et_vq_data *vq_data;
	struct et_squeue *sq;
	u8 *kern_buf;
	ssize_t rv;

	vq_data = (is_mgmt) ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	sq = (is_hp_sq) ? &vq_data->hp_sqs[sq_index] : &vq_data->sqs[sq_index];

	if (!count || count > U16_MAX) {
		pr_err("invalid message size: %ld", count);
		return -EINVAL;
	}

	kern_buf = kzalloc(count, GFP_KERNEL);

	if (!kern_buf)
		return -ENOMEM;

	rv = copy_from_user(kern_buf, ubuf, count);
	if (rv) {
		pr_err("copy_from_user failed\n");
		rv = -ENOMEM;
		goto error;
	}

	rv = et_squeue_push(sq, kern_buf, count);

error:
	kfree(kern_buf);

	return rv;
}

void et_squeue_sync_cb_for_host(struct et_squeue *sq)
{
	u64 head_local;

	mutex_lock(&sq->push_mutex);
	head_local = sq->cb.head;
	et_ioread(sq->cb_mem, 0, (u8 *)&sq->cb, sizeof(sq->cb));

	if (head_local != sq->cb.head)
		pr_err("SQ sync: head mismatched, head_local: %lld, head_remote: %lld",
		       head_local,
		       sq->cb.head);

	mutex_unlock(&sq->push_mutex);
}

void et_squeue_sync_bitmap(struct et_squeue *sq)
{
	et_squeue_sync_cb_for_host(sq);

	// Update sq_bitmap
	mutex_lock(&sq->vq_common->sq_bitmap_mutex);

	if (et_circbuffer_free(&sq->cb) >= atomic_read(&sq->sq_threshold))
		set_bit(sq->index, sq->vq_common->sq_bitmap);
	else
		clear_bit(sq->index, sq->vq_common->sq_bitmap);
	wake_up_interruptible(&sq->vq_common->waitqueue);

	mutex_unlock(&sq->vq_common->sq_bitmap_mutex);
}

bool et_squeue_empty(struct et_squeue *sq)
{
	if (!sq)
		return false;

	et_squeue_sync_cb_for_host(sq);

	if (et_circbuffer_used(&sq->cb) != 0)
		return false;

	return true;
}

ssize_t et_cqueue_copy_to_user(struct et_pci_dev *et_dev,
			       bool is_mgmt,
			       u16 cq_index,
			       char __user *ubuf,
			       size_t count)
{
	struct et_vq_data *vq_data;
	struct et_cqueue *cq;
	struct et_msg_node *msg = NULL;
	ssize_t rv;

	vq_data = is_mgmt ? &et_dev->mgmt.vq_data : &et_dev->ops.vq_data;
	cq = &vq_data->cqs[cq_index];

	if (!ubuf || !count)
		return -EINVAL;

	msg = et_dequeue_msg_node(cq);
	if (!msg || !(msg->msg)) {
		// Empty; no message to POP, returning EAGAIN
		rv = -EAGAIN;
		goto update_cq_bitmap;
	}

	if (count < msg->msg_size) {
		pr_err("User buffer not large enough\n");
		// Enqueue the msg again so the userspace can retry with a larger buffer
		enqueue_msg_node(cq, msg);
		return -EINVAL;
	}

	if (copy_to_user(ubuf, msg->msg, msg->msg_size)) {
		pr_err("failed to copy to user\n");
		rv = -EFAULT;
		goto free_msg_node;
	}

	rv = msg->msg_size;

free_msg_node:
	destroy_msg_node(msg);

update_cq_bitmap:
	// Update cq_bitmap
	mutex_lock(&cq->vq_common->cq_bitmap_mutex);

	if (!et_cqueue_msg_available(cq)) {
		clear_bit(cq->index, cq->vq_common->cq_bitmap);
		wake_up_interruptible(&cq->vq_common->waitqueue);
	}

	mutex_unlock(&cq->vq_common->cq_bitmap_mutex);

	return rv;
}

ssize_t et_cqueue_pop(struct et_cqueue *cq, bool sync_for_host)
{
	struct cmn_header_t header;
	struct et_msg_node *msg_node;
	struct device_mgmt_event_msg_t mgmt_event;
	ssize_t rv;

	mutex_lock(&cq->pop_mutex);

	// Read the message header
	if (!et_circbuffer_pop(&cq->cb,
			       cq->cb_mem,
			       (u8 *)&header,
			       sizeof(header),
			       (sync_for_host) ? ET_CB_SYNC_FOR_HOST : 0)) {
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	// If the size is invalid, the message buffer is corrupt and the
	// system is in a bad state. This should never happen.
	if (!header.size) {
		pr_err("CQ corrupt: invalid size");
		rv = -ENOTRECOVERABLE;
		goto error_unlock_mutex;
	}

	// Check if this is a mgmt event, handle accordingly
	if (header.msg_id >= DEV_MGMT_API_MID_EVENTS_BEGIN &&
	    header.msg_id <= DEV_MGMT_API_MID_EVENTS_END) {
		memcpy((u8 *)&mgmt_event.event_info,
		       (u8 *)&header,
		       sizeof(header));

		if (!et_circbuffer_pop(&cq->cb,
				       cq->cb_mem,
				       (u8 *)&mgmt_event + sizeof(header),
				       header.size - sizeof(header),
				       ET_CB_SYNC_FOR_DEVICE)) {
			rv = -EAGAIN;
			goto error_unlock_mutex;
		}
		mutex_unlock(&cq->pop_mutex);

		rv = et_handle_device_event(cq, &mgmt_event);

		atomic64_inc(
			&cq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
		et_rate_entry_update(
			1,
			&cq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
		atomic64_add(
			header.size + sizeof(header),
			&cq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
		et_rate_entry_update(
			header.size + sizeof(header),
			&cq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

		return rv;
	}

	// Message is for user mode. Save it off.
	msg_node = create_msg_node(header.size + sizeof(header));
	if (!msg_node) {
		rv = -ENOMEM;
		goto error_unlock_mutex;
	}

	memcpy(msg_node->msg, (u8 *)&header, sizeof(header));

	// MMIO msg payload into node memory
	if (!et_circbuffer_pop(&cq->cb,
			       cq->cb_mem,
			       (u8 *)msg_node->msg + sizeof(header),
			       header.size,
			       ET_CB_SYNC_FOR_DEVICE)) {
		destroy_msg_node(msg_node);
		rv = -EAGAIN;
		goto error_unlock_mutex;
	}

	mutex_unlock(&cq->pop_mutex);

	atomic64_inc(&cq->stats.counters[ET_VQ_COUNTER_STATS_MSG_COUNT]);
	et_rate_entry_update(1, &cq->stats.rates[ET_VQ_RATE_STATS_MSG_RATE]);
	atomic64_add(header.size + sizeof(header),
		     &cq->stats.counters[ET_VQ_COUNTER_STATS_BYTE_COUNT]);
	et_rate_entry_update(header.size + sizeof(header),
			     &cq->stats.rates[ET_VQ_RATE_STATS_BYTE_RATE]);

	// Check for MM reset command and complete post reset steps
	if (header.msg_id == DEV_MGMT_API_MID_MM_RESET)
		mm_reset_completion_callback(
			cq,
			(struct device_mgmt_rsp_hdr_t *)msg_node->msg);

	// Enqueue msg node to user msg_list of CQ
	enqueue_msg_node(cq, msg_node);

	mutex_lock(&cq->vq_common->cq_bitmap_mutex);
	set_bit(cq->index, cq->vq_common->cq_bitmap);
	mutex_unlock(&cq->vq_common->cq_bitmap_mutex);

	wake_up_interruptible(&cq->vq_common->waitqueue);

	return header.size;

error_unlock_mutex:
	mutex_unlock(&cq->pop_mutex);

	return rv;
}

void et_cqueue_sync_cb_for_host(struct et_cqueue *cq)
{
	u64 tail_local;

	mutex_lock(&cq->pop_mutex);
	tail_local = cq->cb.tail;
	et_ioread(cq->cb_mem, 0, (u8 *)&cq->cb, sizeof(cq->cb));

	if (tail_local != cq->cb.tail)
		pr_err("CQ sync: tail mismatched, tail_local: %lld, tail_remote: %lld",
		       tail_local,
		       cq->cb.tail);

	mutex_unlock(&cq->pop_mutex);
}

/*
 * Handles vqueue IRQ. The vqueue IRQ signals a a new message in the CQ.
 *
 * This method handles CQ messages for the kernel immediatley, and saves
 * off messages for user mode to be consumed later.
 *
 * User mode messages must not block kernel messages from being processed
 * (e.g if the first msg in the vqueue is for the user and the second is for
 * the kernel, the kernel message should not be stuck in line behind the user
 * message).
 *
 * This method must be tolerant of spurious IRQs (no new msg), and taking an
 * IRQ while messages are still in filght.
 *
 * Reasons it may fire:
 *
 * - The host sent a msg to this vqueue
 *
 * - The host sent a msg to another vqueue, and MSI
 *   multivector support is not available (IRQ is spurious for this vqueue)
 *
 * - The host sent two (or more) messages and two (or more) IRQs, but the ISR
 *   handeled multiple messages in one pass, (follow-on IRQs should be ignored)
 *
 *   Another version of this: the ISR sees the data for the first message, and
 *   only a portion of the data for the second message (rest of data and second
 *   IRQ still in flight)
 *
 * - Perodic wakeup fired (incase IRQs missed). There may be a state update or
 *   msg, there may be a message in flight (should take no action and wait for
 *   next IRQ), or there may be no changes (IRQ is spurious)
 */
void et_cqueue_isr_bottom(struct et_cqueue *cq)
{
	bool sync_for_host = true;

	// Handle all pending messages in the cqueue
	while (et_cqueue_pop(cq, sync_for_host) > 0) {
		// Only sync `circbuffer` the first time
		if (sync_for_host)
			sync_for_host = false;
	}
}
