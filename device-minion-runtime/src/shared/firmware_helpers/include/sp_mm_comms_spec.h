/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file sp_mm_comms_spec.h
    \brief A C header that defines communication messages between MM and SP.
*/
/***********************************************************************/

#ifndef SP_MM_COMMS_SPEC_H
#define SP_MM_COMMS_SPEC_H

#include <inttypes.h>

typedef int16_t mm2sp_mm_recoverable_error_code_e;

/*! \enum mm2sp_mm_recoverable_error_code_e
    \brief MM to SP error codes which can be recovered by MM itself. For these errors,
           mm will just inform SP and SP will increment the counter.
           When the threshold is hit on SP side, SP will inform the Host
*/
enum mm2sp_mm_recoverable_error_code_e {
    MM_HANG_ERROR                     = -1,
    MM_EXCEPTION_ERROR                = -2,
    MM_DMA_CONFIG_ERROR               = -3,
    MM_DMA_TIMEOUT_ERROR              = -4,
    MM_KERNEL_LAUNCH_ERROR            = -5,
    MM_CQ_PUSH_ERROR                  = -6,
    MM_SQ_PROCESSING_ERROR            = -7,
    MM_SQ_BUFFER_ALIGNMENT_ERROR      = -8,
    MM_CMD_BARRIER_TIMEOUT_ERROR      = -9,
    MM_CM_RESERVE_SLOT_ERROR          = -10,
    MM_CM_RESERVE_TIMEOUT_ERROR       = -11,
    MM_CM_KERNEL_ABORT_TIMEOUT_ERROR  = -12,
    MM_MM2CM_CMD_ERROR                = -13,
    MM_CM2MM_CMD_ERROR                = -14,
    MM_CM2MM_KERNEL_LAUNCH_ERROR      = -15,
    MM_CM2MM_KERNEL_EXCEPTION_ERROR   = -16
};

typedef int16_t mm2sp_sp_recoverable_error_code_e;

/*! \enum mm2sp_sp_recoverable_error_code_e
    \brief MM to SP error codes for which MM is not able to recover from error state,
           so it will require intervention from SP and perform the reset sequence
*/
enum mm2sp_sp_recoverable_error_code_e {
    MM_CM2MM_MM_HANG             = -1,
    MM_CM_IFACE_INIT_ERROR       = -2,
    MM_SP_IFACE_INIT_ERROR       = -3,
    MM_CW_INIT_ERROR             = -4,
    MM_SERIAL_INIT_ERROR         = -5,
    MM_CQ_INIT_ERROR             = -6,
    MM_SQ_INIT_ERROR             = -7
};

typedef uint16_t mm2sp_error_type_e;

/*! \enum mm2sp_error_type_e
    \brief MM to SP reported error's type, it specifies that whether a particular
           error is MM recoverable or it needs SP intervention.
*/
enum mm2sp_error_type_e {
    MM_RECOVERABLE               = 0,
    SP_RECOVERABLE
};

/*********************************
    MM <> SP Message IDs
**********************************/

/*! \enum mm_sp_msg_e
    \brief MM to SP commands/message IDs
*/
enum mm_sp_msg_e {
    MM2SP_CMD_OFFSET = 128,
    MM2SP_CMD_ECHO,
    MM2SP_RSP_ECHO,
    MM2SP_CMD_GET_ACTIVE_SHIRE_MASK,
    MM2SP_RSP_GET_ACTIVE_SHIRE_MASK,
    MM2SP_CMD_GET_CM_BOOT_FREQ,
    MM2SP_RSP_GET_CM_BOOT_FREQ,
    MM2SP_EVENT_REPORT_ERROR, /* this is more flexible, the payload now can report error codes in a scalable fashion */
    MM2SP_EVENT_HEARTBEAT
};

/*! \enum sp_mm_msg_e
    \brief SP to MM commands/message IDs
*/
enum sp_mm_msg_e {
    SP2MM_CMD_ECHO = 256,
    SP2MM_RSP_ECHO,
    SP2MM_CMD_UPDATE_FREQ,
    SP2MM_RSP_UPDATE_FREQ,
    SP2MM_CMD_TEARDOWN_MM,
    SP2MM_RSP_TEARDOWN_MM,
    SP2MM_CMD_QUIESCE_TRAFFIC,
    SP2MM_RSP_QUIESCE_TRAFFIC,
    SP2MM_CMD_GET_TRACE_BUFF_CONTROL_STRUCT,
    SP2MM_RSP_GET_TRACE_BUFF_CONTROL_STRUCT,
    SP2MM_CMD_KERNEL_LAUNCH,
    SP2MM_RSP_KERNEL_LAUNCH,
    SP2MM_CMD_GET_DRAM_BW,
    SP2MM_RSP_GET_DRAM_BW
};

/* TODO: We should invent a new cmd and rsp header for the mrt messages
the device api cmd and rsp headers being used now have some redundant
fields */

/*********************************
    MM to SP Command Structures
**********************************/

/*! \struct dev_cmd_hdr_t
    \brief Command header for all messages between MM and SP.
*/
struct dev_cmd_hdr_t {
  uint16_t msg_size;
  uint16_t msg_id;
  int32_t issuing_hart_id; /* Used by MM to SP commands only */
};

/*! \struct mm2sp_echo_cmd_t
    \brief MM to SP Echo command structure
*/
struct mm2sp_echo_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

/*! \struct mm2sp_echo_rsp_t
    \brief MM to SP Echo command's response structure
*/
struct mm2sp_echo_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

/*! \struct mm2sp_get_active_shire_mask_cmd_t
    \brief MM to SP command strutcure for get active shire command
*/
struct mm2sp_get_active_shire_mask_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*! \struct mm2sp_get_active_shire_mask_rsp_t
    \brief MM to SP command strutcure for get active shire command'
           response
*/
struct mm2sp_get_active_shire_mask_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint64_t  active_shire_mask;
};

/*! \struct mm2sp_get_cm_boot_freq_cmd_t
    \brief MM to SP command strutcure for get Compute Minion Boot
           frequency
*/
struct mm2sp_get_cm_boot_freq_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*! \struct mm2sp_get_cm_boot_freq_rsp_t
    \brief MM to SP command strutcure for get Compute Minion Boot
           frequency command's response.
*/
struct mm2sp_get_cm_boot_freq_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  cm_boot_freq;
};

/*! \struct mm2sp_report_error_event_t
    \brief MM to SP event structure to report an error.
*/
struct mm2sp_report_error_event_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint16_t error_type;
  int16_t error_code;
};

/*! \struct mm2sp_heartbeat_event_t
    \brief MM to SP event structure to report periodic heartbeat.
*/
struct mm2sp_heartbeat_event_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*********************************
    SP to MM Command Structures
**********************************/

/*! \struct sp2mm_echo_cmd_t
    \brief SP to MM Echo command structure
*/
struct sp2mm_echo_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

/*! \struct sp2mm_echo_rsp_t
    \brief SP to MM Echo command's response structure
*/
struct sp2mm_echo_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

/*! \struct sp2mm_update_freq_cmd_t
    \brief SP to MM command strutcure to update MM frequency.
*/
struct sp2mm_update_freq_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  freq;
};

/*! \struct sp2mm_update_freq_rsp_t
    \brief SP to MM response strutcure to update MM frequency command.
*/
struct sp2mm_update_freq_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

/*! \struct sp2mm_teardown_mm_cmd_t
    \brief SP to MM command strutcure to tear down MM.
*/
struct sp2mm_teardown_mm_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*! \struct sp2mm_teardown_mm_rsp_t
    \brief SP to MM response strutcure to tear down command.
*/
struct sp2mm_teardown_mm_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

/*! \struct sp2mm_quiesce_traffic_cmd_t
    \brief SP to MM command strutcure for quiesce traffic.
*/
struct sp2mm_quiesce_traffic_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*! \struct sp2mm_quiesce_traffic_rsp_t
    \brief SP to MM response strutcure for quiesce traffic command.
*/
struct sp2mm_quiesce_traffic_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

/*! \struct sp2mm_kernel_launch_cmd_t
    \brief SP to MM command strutcure for Kernel Launch command.
*/
struct sp2mm_kernel_launch_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint64_t  shire_mask;
  void *args;
};

/*! \struct sp2mm_kernel_launch_rsp_t
    \brief SP to MM response strutcure for Kernel Launch command.
*/
struct sp2mm_kernel_launch_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

/*! \struct sp2mm_get_dram_bw_cmd_t
    \brief SP to MM command strutcure for Get DRAM BW command.
*/
struct sp2mm_get_dram_bw_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

/*! \struct sp2mm_get_dram_bw_rsp_t
    \brief SP to MM response strutcure for Get DRAM BW command.
*/
struct sp2mm_get_dram_bw_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  read_bw;
  uint32_t  write_bw;
};

/*! \def  MM2SP_CMD
    \brief Helper macro to construct MM to SP commands
*/
#define MM2SP_CMD(cmd_var, cmd_id, cmd_size, issuing_hart_id) \
    cmd_var.msg_hdr.msg_id = cmd_id; cmd_var.msg_hdr.msg_size = cmd_size; \
    cmd_var.msg_hdr.issuing_hart_id = (uint16_t) issuing_hart_id

#endif /* MM_SP_CMD_SPEC_H */
